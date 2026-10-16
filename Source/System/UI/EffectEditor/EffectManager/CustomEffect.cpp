#include "CustomEffect.h"
#include "System/Graphic/Graphics.h"
#include "System/Core/Mathf.h"
#include "System/Shader/meshVFX/meshVFX.h"
#include "System/Resource/ModelResourceManager/ModelResourceManager.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"

#include <ctime>
#include <cstdlib>


// 初期化
void CustomEffect::Initialize(ID3D11Device* device, const EffectConfig& config)
{
    // エフェクトの設定を読み込む
    this->config = config;

    // ノードの数だけ配列の枠を確保する
    runTimeNodes.resize(config.nodes.size());

    for (size_t i = 0; i < config.nodes.size(); ++i)
    {
        auto& nodeConfig = config.nodes[i];
        auto& rNode = runTimeNodes[i];

        rNode.config = nodeConfig;

        if (nodeConfig.type == RenderType::Particle)
        {
            rNode.particleManager = std::make_unique<Compute_Particle>(device);
        }
        // メッシュノードなら、そのノードのパスでモデルを生成
        else if (nodeConfig.type == RenderType::Mesh && !nodeConfig.meshPath.empty())
        {
            rNode.meshModel = ModelResourceManager::Instance().GetOrLoad(device, nodeConfig.meshPath);

            if (!nodeConfig.texturePath.empty())
            {
                rNode.shaderResourceView = TextureResourceManager::Instance().GetOrLoad(device, nodeConfig.texturePath);
            }

            if (!nodeConfig.noisePath.empty())
            {
                rNode.noiseShaderResourceView = TextureResourceManager::Instance().GetOrLoad(device, nodeConfig.noisePath);
            }

        }
        // トレイルノードなら、個別にTrailインスタンスを生成
        else if (nodeConfig.type == RenderType::Trail)
        {
            rNode.trail = std::make_shared<Trail>();
            rNode.trail->Initialize();
        }
    }
}

// 再生中に設定値だけを差し替える（エディターのプレビュー用）
bool CustomEffect::ApplyConfig(const EffectConfig& newConfig)
{
    // ノード数が変わっていたら作り直しが必要
    if (newConfig.nodes.size() != runTimeNodes.size()) return false;

    for (size_t i = 0; i < newConfig.nodes.size(); ++i)
    {
        const auto& newNode = newConfig.nodes[i];
        const auto& oldNode = runTimeNodes[i].config;

        // リソースに関わる値が変わっていたら作り直しが必要
        if (newNode.type != oldNode.type ||
            newNode.meshPath != oldNode.meshPath ||
            newNode.texturePath != oldNode.texturePath ||
            newNode.noisePath != oldNode.noisePath)
        {
            return false;
        }
    }

    // 数値だけの変更なので、そのまま差し替える
    config = newConfig;
    for (size_t i = 0; i < newConfig.nodes.size(); ++i)
    {
        runTimeNodes[i].config = newConfig.nodes[i];
    }

    return true;
}

// エフェクト再生
void CustomEffect::Play(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale)
{
    this->position = position;
    this->scale = scale;
    this->angle = angle;

    // 初回の計算で暴発しないようにする
    prevPosition = position;

    effectTimer = 0.0f; // タイマーを0にする
    isPlaying = true; // 再生中にする

    for (auto& node : runTimeNodes)
    {
        node.localTime = 0.0f;
        node.started = false;
        node.emitting = false;
        node.finished = false;
    }
}

// エフェクト更新
void CustomEffect::Update(float elapsedTime)
{
    if (!isPlaying) return;

    // タイマー更新
    effectTimer += elapsedTime;

    bool allFinished = true; // 全部のノードが終わったか

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

    // 移動した差分値を求める
    delta.x = this->position.x - prevPosition.x;
    delta.y = this->position.y - prevPosition.y;
    delta.z = this->position.z - prevPosition.z;

    // 前回の位置を保存する
    prevPosition = this->position;

    for (auto& node : runTimeNodes)
    {
        // UVスクロールの時間を進める
        node.currentUvOffset.x += node.config.uvScrollSpeed.x * elapsedTime;
        node.currentUvOffset.y += node.config.uvScrollSpeed.y * elapsedTime;

        node.currentUvOffset.x = fmodf(node.currentUvOffset.x, 1.0f);
        node.currentUvOffset.y = fmodf(node.currentUvOffset.y, 1.0f);

        // パーティクルの計算を進める
        if (node.particleManager)
        {
            node.particleManager->Update(dc, delta, elapsedTime);
        }

        if (!node.config.visible || node.finished) continue; // 非表示またはそのノードが終わったら飛ばす

        allFinished = false; // まだ終わってないノードある


        if (!node.started)
        {
            if (effectTimer < node.config.delayTime) continue; // 待機時間内ならスキップ

            // 待機時間が終わったらスタートする
            node.started = true;
            node.localTime = effectTimer - node.config.delayTime;
            BeginNodeCycle(node); // エフェクトの設定をする
        }
        else
        {
            node.localTime += elapsedTime; // 既に始まってる
        }

        // ループ処理
        const bool isParticle = (node.config.type == RenderType::Particle);

        // パーティクルはエフェクト全体の生存時間で、それ以外は特殊で寿命で判定を取る
        const float cycle = isParticle ? node.config.duration : node.config.lifeTime; 

        // 周期が0以上でノードごとの再生時間が周期よりも大きかったら
        if (cycle > 0.0f && node.localTime >= cycle) // 0は無限とする
        {
            // ループを設定したら
            if (node.config.isLoop)
            {
                // ループ：巻き戻して次の周期へ
                node.localTime = fmodf(node.localTime, cycle);
                BeginNodeCycle(node); // 周期ごとに生成する
            }
            else if (isParticle)
            {
                // ループしないパーティクルは発生を止めて最後のパーティクルが消えるまで待つ
                node.emitting = false;
                if (node.localTime >= cycle + node.config.lifeTime)
                {
                    node.finished = true;
                }
            }
            else
            {
               // ループしないメッシュ・トレイル：終わり
                node.finished = true;
                continue;
            }
        }

        // メッシュVFX
       if (node.config.type == RenderType::Mesh)
        {
            // メッシュノードごとの発生からの経過時間を計算
            float activeTime = node.localTime;
            float t = activeTime; // 物理計算用の時間

            if (node.meshModel)
            {
                DirectX::XMFLOAT3 finalScale = { 1, 1, 1 };
                switch (node.config.scaleMode)
                {
                case ScaleMode::Fixed:
                    // 拡大時間経過で大きく/小さくする
                    // scale = 初期拡大 + (拡大速度 * t) + (0.5 * 拡大加速度 * t^2)　物理の計算らしい
                    finalScale.x = this->scale.x * (node.config.scale.x + (node.config.scaleVelocity.x * activeTime) + (0.5f * node.config.scaleAcceleration.x * activeTime * activeTime));
                    finalScale.y = this->scale.y * (node.config.scale.y + (node.config.scaleVelocity.y * activeTime) + (0.5f * node.config.scaleAcceleration.y * activeTime * activeTime));
                    finalScale.z = this->scale.z * (node.config.scale.z + (node.config.scaleVelocity.z * activeTime) + (0.5f * node.config.scaleAcceleration.z * activeTime * activeTime));

                    break;
                case ScaleMode::UniformEasing:
                {
                    float t = activeTime / node.config.lifeTime;
                    float easeT = Mathf::EaseCurve(t, node.config.scaleUniformStartSpeed, node.config.scaleUniformEndSpeed);

                    float uniform = Mathf::Lerp(node.meshUniformStartScale, node.meshUniformEndScale, easeT);
                    finalScale.x = this->scale.x * uniform;
                    finalScale.y = this->scale.y * uniform;
                    finalScale.z = this->scale.z * uniform;
                }
                break;
                }

                // マイナスにならないようにガード
                finalScale.x = (std::max)(0.0f, finalScale.x);
                finalScale.y = (std::max)(0.0f, finalScale.y);
                finalScale.z = (std::max)(0.0f, finalScale.z);

                DirectX::XMMATRIX S = DirectX::XMMatrixScaling(finalScale.x, finalScale.y, finalScale.z);

                // 回転（初期回転 ＋ 初期角度 ＋ 自転・回転速度 ＋ 回転加速度）
                DirectX::XMFLOAT3 currentRot = {
                    this->angle.x + (node.config.rotation.x + (node.config.rotationVelocity.x * t) + (0.5f * node.config.rotationAcceleration.x * t * t)),
                    this->angle.y + (node.config.rotation.y + (node.config.rotationVelocity.y * t) + (0.5f * node.config.rotationAcceleration.y * t * t)),
                    this->angle.z + (node.config.rotation.z + (node.config.rotationVelocity.z * t) + (0.5f * node.config.rotationAcceleration.z * t * t))
                };
                DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(currentRot.x),
                    DirectX::XMConvertToRadians(currentRot.y),
                    DirectX::XMConvertToRadians(currentRot.z)
                );

                // 位置（Play時のエフェクト発生位置 ＋ ノード初期位置 ＋ 移動速度 ＋ 移動加速度）
                DirectX::XMFLOAT3 currentPos = {
                    this->position.x + node.config.position.x + (node.config.positionVelocity.x * t) + (0.5f * node.config.positionAcceleration.x * t * t),
                    this->position.y + node.config.position.y + (node.config.positionVelocity.y * t) + (0.5f * node.config.positionAcceleration.y * t * t),
                    this->position.z + node.config.position.z + (node.config.positionVelocity.z * t) + (0.5f * node.config.positionAcceleration.z * t * t)
                };
                DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(currentPos.x, currentPos.y, currentPos.z);

                // 行列を合成してモデルに適用
                DirectX::XMFLOAT4X4 worldTransform;
                DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);

                node.worldTransform = worldTransform;
            }
        }
        // トレイル
        else if (node.config.type == RenderType::Trail)
        {
            DirectX::XMFLOAT3 root = {
                this->position.x,
                this->position.y,
                this->position.z
            };

            DirectX::XMFLOAT3 tip = {
                root.x,
                root.y + node.config.width,
                root.z
            };

            if (node.trail)
            {
                node.trail->SetWidthScales(node.config.widthStart, node.config.widthMiddle, node.config.widthEnd);
                node.trail->Update(root, tip);
            }
        }
    } 

    EmitParticles(elapsedTime);

    if (allFinished)
    {
        isPlaying = false; // 再生終了
    }
}

// エフェクト描画
void CustomEffect::Render(RenderContext& rc)
{
    if (isPlaying)
    {
        for (const auto& node : runTimeNodes)
        {
            if (!node.config.visible || node.finished) continue; // 非表示または終了していたら飛ばす

            // パーティクルの描画
            if (node.particleManager)
            {
                node.particleManager->Render(rc);
            }

            if (node.config.type == RenderType::Trail && node.trail)
            {
                node.trail->Render(rc);
            }
        }
    }
}

// メッシュVFX描画
void CustomEffect::RenderMeshVFX(meshVFX* shader, RenderContext& rc)
{
    if (!isPlaying) return;
    if (!shader) return;

    ID3D11DeviceContext* dc = rc.deviceContext;

    for (auto& rNode : runTimeNodes)
    {
        if (isPlaying && rNode.meshModel)
        {
            if (!rNode.config.visible || rNode.finished || !rNode.started) continue;

            if (rNode.config.type == RenderType::Mesh)
            {
                float activeTime = rNode.localTime;

                // テクスチャをセット
                shader->SetCurrentTextures(rNode.shaderResourceView.Get(), rNode.noiseShaderResourceView.Get());
                shader->SetDistortionPower(rNode.config.distortionPower);

                // フェードインの計算
                float alpha = 1.0f;
                if (rNode.config.IsFadeIn && rNode.config.fadeInTime > 0.0001f && activeTime <= rNode.config.fadeInTime)
                {
                    // 現在の進行度を求める
                    float t = activeTime / rNode.config.fadeInTime;
                    alpha = Mathf::EaseCurve(t, rNode.config.fadeInStartSpeed, rNode.config.fadeInEndSpeed);
                }
                // フェードアウトの計算
                else if (rNode.config.IsFadeOut && rNode.config.fadeOutTime > 0.0001f)
                {
                    // 進行度を求める
                    float fadeStartTime = rNode.config.lifeTime - rNode.config.fadeOutTime;

                    if (activeTime >= fadeStartTime)
                    {
                        float t = (activeTime - fadeStartTime) / rNode.config.fadeOutTime; // 0.0 ~ 1.0 に正規化
                        alpha = 1.0f - Mathf::EaseCurve(t, rNode.config.fadeOutStartSpeed, rNode.config.fadeOutEndSpeed); // 1.0を引いて1.0～0.0のイージングにする
                    }
                }

                // 0.0～1.0の範囲に収める
                alpha = Mathf::Clamp(alpha);

                DirectX::XMFLOAT2 noiseUvOffset = {
                    rNode.config.noiseUvOffset.x * activeTime,
                    rNode.config.noiseUvOffset.y * activeTime
                };
                shader->SetNoiseUvOffset(noiseUvOffset);
                shader->SetBlendMode(rNode.config.blendMode);

                // タイプごとに色の計算をする
                DirectX::XMFLOAT4 finalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                switch (rNode.config.colorMode)
                {
                case ColorMode::Fixed: // 固定色
                    finalColor = rNode.config.color;

                    break;
                case ColorMode::Random: // ランダム
                    finalColor = rNode.meshRandomColor;

                    break;
                case ColorMode::Easing: // イージング
                {
                    // 進行度を求める
                    float t = activeTime / rNode.config.lifeTime;

                    // イージング速度を求める
                    float easeT = Mathf::EaseCurve(t, rNode.config.colorStartSpeed, rNode.config.colorEndSpeed);

                    // Lerpを掛けて進行度にあ和得て色を変化させる
                    finalColor.x = Mathf::Lerp(rNode.meshStartColor.x, rNode.meshEndColor.x, easeT);
                    finalColor.y = Mathf::Lerp(rNode.meshStartColor.y, rNode.meshEndColor.y, easeT);
                    finalColor.z = Mathf::Lerp(rNode.meshStartColor.z, rNode.meshEndColor.z, easeT);
                    finalColor.w = Mathf::Lerp(rNode.meshStartColor.w, rNode.meshEndColor.w, easeT);

                    break;
                }
                }

                finalColor.w *= alpha;
                shader->SetColor(finalColor); // このノード固有の色
                shader->SetUvOffset(rNode.currentUvOffset);


                for (const auto& mesh : rNode.meshModel->GetMeshes())
                {
                    // 頂点バッファ・インデックスバッファをセット
                    UINT stride = sizeof(Model::Vertex);
                    UINT offset = 0;
                    dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
                    dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                    // 行列とパラメータを定数バッファに更新して描画コール
                    shader->SetWorldTransForm(rNode.worldTransform);
                    shader->Update(rc, mesh);
                    dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
                }
            }
        }
    }
}

// GUI描画
void CustomEffect::DrawGUI()
{

}

// エフェクト終了関数
bool CustomEffect::IsFinished()
{
	return !isPlaying;
}

// 停止関数
void CustomEffect::Stop()
{
    isPlaying = false;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

    for (auto& node : runTimeNodes)
    {
        if (node.particleManager)
        {
            node.particleManager->Clear(dc);
        }
    }

}

// パーティクル放出
void CustomEffect::EmitParticles(float elapsedTime)
{
    // 全ノードを順番にチェックして発生させる
    for (auto& node : runTimeNodes)
    {
        // パーティクルノード以外、または非表示ノードはスキップ
        if (node.config.type != RenderType::Particle) continue;
        if (!node.config.visible || node.finished) continue;

        auto& pConfig = node.config;

        if (!node.emitting) continue; // パーティクル生成していないときは処理を飛ばす

        // ノード個別のタイマーを進める
        node.spawnTimer += elapsedTime;

        float rand = Mathf::RandFloat();

        if (node.spawnTimer >=pConfig.spawnInterval)
        {
            node.spawnTimer -=pConfig.spawnInterval;

            // そのノードのテクスチャをGPUにセット（マルチテクスチャ対応！）
            if (node.particleManager && !pConfig.texturePath.empty())
            {
                node.particleManager->SetTexture(pConfig.texturePath, { (UINT)pConfig.splitCount[0], (UINT)pConfig.splitCount[1] });
            }

            // 生成数分 for 文で回す
            for (int i = 0; i <pConfig.emitCount; i++)
            {
                compute_particle_system::emit_particle_data data;
                data.parameter = { 0.0f, 0.0f, 0.0f, 0.0f };

                data.parameter.x = static_cast<float>(pConfig.spriteType);
                data.parameter.y =pConfig.lifeTime;
                data.parameter.z =pConfig.uvScrollSpeed.x;
                data.parameter.w =pConfig.uvScrollSpeed.y;

                // フェードのスピードを設定する
                int packedSpeed =
                    (pConfig.fadeInStartSpeed)
                    | (pConfig.fadeInEndSpeed << 4)
                    | (pConfig.fadeOutStartSpeed << 8)
                    | (pConfig.fadeOutEndSpeed << 12);

                data.fade_param.x = pConfig.IsFadeIn  ? pConfig.fadeInTime : 0.0f;
                data.fade_param.y = pConfig.IsFadeOut ? pConfig.fadeOutTime : 0.0f;
                data.fade_param.z = pConfig.lifeTime;
                data.fade_param.w = static_cast<float>(packedSpeed);


                // 位置
                data.position = {
                  this->position.x + pConfig.position.x + (pConfig.positionRange.x * Mathf::RandFloat()),
                  this->position.y + pConfig.position.y + (pConfig.positionRange.y * Mathf::RandFloat()),
                  this->position.z + pConfig.position.z + (pConfig.positionRange.z * Mathf::RandFloat()),
                   1.0f
                };
                // 初速度
                data.posVelocity = {
                   pConfig.positionVelocity.x + (pConfig.positionVelocityRange.x * Mathf::RandFloat()),
                   pConfig.positionVelocity.y + (pConfig.positionVelocityRange.y * Mathf::RandFloat()),
                   pConfig.positionVelocity.z + (pConfig.positionVelocityRange.z * Mathf::RandFloat()),
                    0.0f
                };
                // 加速度
                data.posAcceleration = {
                   pConfig.positionAcceleration.x + (pConfig.positionAccelerationRange.x * Mathf::RandFloat()),
                   pConfig.positionAcceleration.y + (pConfig.positionAccelerationRange.y * Mathf::RandFloat()),
                   pConfig.positionAcceleration.z + (pConfig.positionAccelerationRange.z * Mathf::RandFloat()),
                    0.0f
                };

                // 回転
                data.rotation = {
                    DirectX::XMConvertToRadians(this->angle.x + (pConfig.rotation.x + (pConfig.rotationRange.x * Mathf::RandFloat()))),
                    DirectX::XMConvertToRadians(this->angle.y + (pConfig.rotation.y + (pConfig.rotationRange.y * Mathf::RandFloat()))),
                    DirectX::XMConvertToRadians(this->angle.z + (pConfig.rotation.z + (pConfig.rotationRange.z * Mathf::RandFloat()))),
                    pConfig.isBillboard ? 1.0f : 0.0f
                };
                data.rotVelocity = {
                    DirectX::XMConvertToRadians(pConfig.rotationVelocity.x + (pConfig.rotationVelocityRange.x * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationVelocity.y + (pConfig.rotationVelocityRange.y * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationVelocity.z + (pConfig.rotationVelocityRange.z * Mathf::RandFloat())),
                    0.0f
                };
                data.rotAcceleration = {
                    DirectX::XMConvertToRadians(pConfig.rotationAcceleration.x + (pConfig.rotationAccelerationRange.x * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationAcceleration.y + (pConfig.rotationAccelerationRange.y * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationAcceleration.z + (pConfig.rotationAccelerationRange.z * Mathf::RandFloat())),
                    0.0f
                };

                int scaleEasingSpeed =
                    (pConfig.scaleUniformStartSpeed)
                    | (pConfig.scaleUniformEndSpeed << 4);

                // 拡大
                switch (pConfig.scaleMode)
                {
                case ScaleMode::Fixed: // 固定
                    data.scale = {
                        std::max(0.0f, this->scale.x * (pConfig.scale.x + (pConfig.scaleRange.x * Mathf::RandFloat()))),
                        std::max(0.0f, this->scale.y * (pConfig.scale.y + (pConfig.scaleRange.y * Mathf::RandFloat()))),
                        1.0f, 0.0f
                    };
                    data.scaleVelocity = {
                        pConfig.scaleVelocity.x + (pConfig.scaleVelocityRange.x * Mathf::RandFloat()),
                        pConfig.scaleVelocity.y + (pConfig.scaleVelocityRange.y * Mathf::RandFloat()),
                        pConfig.scaleVelocity.z + (pConfig.scaleVelocityRange.z * Mathf::RandFloat()),
                        0.0f
                    };
                    data.scaleAcceleration = {
                        pConfig.scaleAcceleration.x + (pConfig.scaleAccelerationRange.x * Mathf::RandFloat()),
                        pConfig.scaleAcceleration.y + (pConfig.scaleAccelerationRange.y * Mathf::RandFloat()),
                        pConfig.scaleAcceleration.z + (pConfig.scaleAccelerationRange.z * Mathf::RandFloat()),
                        0.0f
                    };

                    data.scale_param.x = -1.0f; // イージング無しのフラグ

                    break;
                case ScaleMode::UniformEasing: // 単一イージング

                    // 中心 ± 振れ幅の範囲でランダムに実際の値を決める
                    float actualStart = pConfig.scaleUniformStart
                        + Mathf::RandomRange(-pConfig.scaleUniformStartRandom, pConfig.scaleUniformStartRandom);
                    float actualEnd = pConfig.scaleUniformEnd
                        + Mathf::RandomRange(-pConfig.scaleUniformEndRandom, pConfig.scaleUniformEndRandom);

                    data.scaleUniformStart = this->scale.x * actualStart;
                    data.scaleUniformEnd = this->scale.x * actualEnd;
                    data.scale_param.x = scaleEasingSpeed;
                    data.scale_param.y = pConfig.lifeTime;
                    break;
                }

                int colorEasingSpeed =
                    (pConfig.colorStartSpeed)
                    | (pConfig.colorEndSpeed << 4);

                // 色
                switch (pConfig.colorMode)
                {
                case ColorMode::Fixed: // 固定
                    data.color = pConfig.color;
                    data.start_color = pConfig.color;
                    data.end_color = pConfig.color;
                    data.color_param.x = -1.0f; // イージング無しのフラグ
                    data.color_param.y = pConfig.lifeTime;

                    break;

                case ColorMode::Random: // ランダム
                    // 色をランダムで取得してcolorに入れる
                    data.color.x = Mathf::RandomRange(pConfig.colorMin.x, pConfig.colorMax.x);
                    data.color.y = Mathf::RandomRange(pConfig.colorMin.y, pConfig.colorMax.y);
                    data.color.z = Mathf::RandomRange(pConfig.colorMin.z, pConfig.colorMax.z);
                    data.color.w = Mathf::RandomRange(pConfig.colorMin.w, pConfig.colorMax.w);

                    data.start_color = data.color;
                    data.end_color = data.color;
                    data.color_param.x = -1.0f;
                    data.color_param.y = pConfig.lifeTime;


                    break;

                case ColorMode::Easing: // イージング
                {
                    // 始点の色を指定した範囲でランダムに取得
                    data.start_color.x = Mathf::RandomRange(pConfig.colorStartMin.x, pConfig.colorStartMax.x);
                    data.start_color.y = Mathf::RandomRange(pConfig.colorStartMin.y, pConfig.colorStartMax.y);
                    data.start_color.z = Mathf::RandomRange(pConfig.colorStartMin.z, pConfig.colorStartMax.z);
                    data.start_color.w = Mathf::RandomRange(pConfig.colorStartMin.w, pConfig.colorStartMax.w);

                    // 終点の色を指定した範囲でランダムに取得
                    data.end_color.x = Mathf::RandomRange(pConfig.colorEndMin.x, pConfig.colorEndMax.x);
                    data.end_color.y = Mathf::RandomRange(pConfig.colorEndMin.y, pConfig.colorEndMax.y);
                    data.end_color.z = Mathf::RandomRange(pConfig.colorEndMin.z, pConfig.colorEndMax.z);
                    data.end_color.w = Mathf::RandomRange(pConfig.colorEndMin.w, pConfig.colorEndMax.w);

                    data.color_param.x = colorEasingSpeed;
                    data.color_param.y = pConfig.lifeTime;

                    break;
                }
                }

                node.particleManager->Emit(data);
            }
        }
    }
}

void CustomEffect::BeginNodeCycle(RuntimeNode& node)
{
    if (node.config.type == RenderType::Particle)
    {
        node.spawnTimer = node.config.spawnInterval;
        node.emitting = true;
    }

    if (node.config.type == RenderType::Mesh && node.config.scaleMode == ScaleMode::UniformEasing)
    {
        // 始点の拡大率を計算する
        node.meshUniformStartScale = node.config.scaleUniformStart + Mathf::RandomRange(-node.config.scaleUniformStartRandom, node.config.scaleUniformStartRandom); // 始点の中心 + 振れ幅
        node.meshUniformEndScale = node.config.scaleUniformEnd + Mathf::RandomRange(-node.config.scaleUniformEndRandom, node.config.scaleUniformEndRandom); // 終点の中心 + 振れ幅
    }

    if (node.config.type == RenderType::Mesh && node.config.colorMode == ColorMode::Random)
    {
        // 色をランダムで取得する
        node.meshRandomColor.x = Mathf::RandomRange(node.config.colorMin.x, node.config.colorMax.x);
        node.meshRandomColor.y = Mathf::RandomRange(node.config.colorMin.y, node.config.colorMax.y);
        node.meshRandomColor.z = Mathf::RandomRange(node.config.colorMin.z, node.config.colorMax.z);
        node.meshRandomColor.w = Mathf::RandomRange(node.config.colorMin.w, node.config.colorMax.w);
    }
    else if (node.config.type == RenderType::Mesh && node.config.colorMode == ColorMode::Easing)
    {
        // 始点の色を指定した範囲でランダムに取得
        node.meshStartColor.x = Mathf::RandomRange(node.config.colorStartMin.x, node.config.colorStartMax.x);
        node.meshStartColor.y = Mathf::RandomRange(node.config.colorStartMin.y, node.config.colorStartMax.y);
        node.meshStartColor.z = Mathf::RandomRange(node.config.colorStartMin.z, node.config.colorStartMax.z);
        node.meshStartColor.w = Mathf::RandomRange(node.config.colorStartMin.w, node.config.colorStartMax.w);

        // 終点の色を指定した範囲でランダムに取得
        node.meshEndColor.x = Mathf::RandomRange(node.config.colorEndMin.x, node.config.colorEndMax.x);
        node.meshEndColor.y = Mathf::RandomRange(node.config.colorEndMin.y, node.config.colorEndMax.y);
        node.meshEndColor.z = Mathf::RandomRange(node.config.colorEndMin.z, node.config.colorEndMax.z);
        node.meshEndColor.w = Mathf::RandomRange(node.config.colorEndMin.w, node.config.colorEndMax.w);
    }
}
