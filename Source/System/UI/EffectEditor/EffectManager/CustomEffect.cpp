#include "CustomEffect.h"
#include "System/Graphic/Graphics.h"
#include "System/Core/Mathf.h"
#include "System/Renderer/Shader/meshVFX/meshVFX.h"
#include "System/Resource/ModelResourceManager/ModelResourceManager.h"

#include <ctime>
#include <cstdlib>


// 初期化
void CustomEffect::Initialize(ID3D11Device* device, const EffectConfig& config)
{
    // エフェクトの設定を読み込む
    this->config = config;

    for (const auto& nodeConfig : config.nodes)
    {
        RuntimeNode rNode;
        rNode.config = nodeConfig;

        // メッシュノードなら、そのノードのパスでモデルを生成
        if (nodeConfig.type == RenderType::Mesh && !nodeConfig.meshPath.empty())
        {
            rNode.meshModel = ModelResourceManager::Instance().GetOrLoad(device, nodeConfig.meshPath);

            if (!nodeConfig.texturePath.empty())
            {
                D3D11_TEXTURE2D_DESC texture2d_desc;
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = GpuResourceUtils::LoadTexture(device, nodeConfig.texturePath.c_str(), srv.GetAddressOf(), &texture2d_desc);

                rNode.shaderResourceView = srv;
            }

            if (!nodeConfig.noisePath.empty())
            {
                D3D11_TEXTURE2D_DESC texture2d_desc;
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = GpuResourceUtils::LoadTexture(device, nodeConfig.noisePath.c_str(), srv.GetAddressOf(), &texture2d_desc);

                rNode.noiseShaderResourceView = srv;
            }

        }
        // トレイルノードなら、個別にTrailインスタンスを生成
        else if (nodeConfig.type == RenderType::Trail)
        {
            rNode.trail = std::make_shared<Trail>();
            rNode.trail->Initialize();
        }

        runTimeNodes.push_back(rNode);
    }

    srand(static_cast<unsigned int>(time(nullptr)));
}

// エフェクト再生
void CustomEffect::Play(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale)
{
	this->position = position;
	this->scale = scale;
	this->angle = angle;

	effectTimer = 0.0f; // タイマーを0にする
	isPlaying = true; // 再生中にする

    for (auto& node : runTimeNodes)
    {
        if (node.config.type == RenderType::Particle)
        {
            node.config.spawnTimer = node.config.spawnInterval;
        }

        if (node.config.type == RenderType::Mesh && node.config.scaleMode == ScaleMode::UniformEasing)
        {
            // 始点の拡大率を計算する
            node.config.meshUniformStartScale = node.config.scaleUniformStart + Mathf::RandomRange(-node.config.scaleUniformStartRandom, node.config.scaleUniformStartRandom); // 始点の中心 + 振れ幅
            node.config.meshUniformEndScale = node.config.scaleUniformEnd + Mathf::RandomRange(-node.config.scaleUniformEndRandom, node.config.scaleUniformEndRandom); // 終点の中心 + 振れ幅
        }

        if (node.config.type == RenderType::Mesh && node.config.colorMode == ColorMode::Random)
        {
            // 色をランダムで取得する
            node.config.meshRandomColor.x = Mathf::RandomRange(node.config.colorMin.x, node.config.colorMax.x);
            node.config.meshRandomColor.y = Mathf::RandomRange(node.config.colorMin.y, node.config.colorMax.y);
            node.config.meshRandomColor.z = Mathf::RandomRange(node.config.colorMin.z, node.config.colorMax.z);
            node.config.meshRandomColor.w = Mathf::RandomRange(node.config.colorMin.w, node.config.colorMax.w);
        }
        else if (node.config.type == RenderType::Mesh && node.config.colorMode == ColorMode::Easing)
        {
            // 始点の色を指定した範囲でランダムに取得
            node.config.meshStartColor.x = Mathf::RandomRange(node.config.colorStartMin.x, node.config.colorStartMax.x);
            node.config.meshStartColor.y = Mathf::RandomRange(node.config.colorStartMin.y, node.config.colorStartMax.y);
            node.config.meshStartColor.z = Mathf::RandomRange(node.config.colorStartMin.z, node.config.colorStartMax.z);
            node.config.meshStartColor.w = Mathf::RandomRange(node.config.colorStartMin.w, node.config.colorStartMax.w);

            // 終点の色を指定した範囲でランダムに取得
            node.config.meshEndColor.x = Mathf::RandomRange(node.config.colorEndMin.x, node.config.colorEndMax.x);
            node.config.meshEndColor.y = Mathf::RandomRange(node.config.colorEndMin.y, node.config.colorEndMax.y);
            node.config.meshEndColor.z = Mathf::RandomRange(node.config.colorEndMin.z, node.config.colorEndMax.z);
            node.config.meshEndColor.w = Mathf::RandomRange(node.config.colorEndMin.w, node.config.colorEndMax.w);
        }
    }
}

// エフェクト更新
void CustomEffect::Update(float elapsedTime, std::unique_ptr<Compute_Particle>& manager)
{
    if (!isPlaying) return;

    // タイマー更新
    effectTimer += elapsedTime;

    float maxDuration = 0.0f;

    for (auto& node : runTimeNodes)
    {
        if (!node.config.visible) continue;

        // UVスクロールの時間を進める
        node.currentUvOffset.x += node.config.uvScrollSpeed.x * elapsedTime;
        node.currentUvOffset.y += node.config.uvScrollSpeed.y * elapsedTime;

        node.currentUvOffset.x = fmodf(node.currentUvOffset.x, 1.0f);
        node.currentUvOffset.y = fmodf(node.currentUvOffset.y, 1.0f);

        // パーティクル
        if (node.config.type == RenderType::Particle)
        {
            if (node.config.duration > maxDuration) maxDuration = node.config.duration;
        }
        // メッシュVFX
        else if (node.config.type == RenderType::Mesh)
        {
            if (effectTimer < node.config.delayTime) continue;

            // メッシュノードごとの発生からの経過時間を計算
            float activeTime = effectTimer - node.config.delayTime;
            float t = activeTime; // 物理計算用の時間

            if (node.meshModel)
            {
                DirectX::XMFLOAT3 finalScale = { 1, 1, 1 };
                switch (node.config.scaleMode)
                {
                case ScaleMode::Fixed:
                    // 拡大時間経過で大きく/小さくする
                    // scale = 初期拡大 + (拡大速度 * t) + (0.5 * 拡大加速度 * t^2)　物理の計算らしい
                    finalScale.x = node.config.scale.x + (node.config.scaleVelocity.x * activeTime) + (0.5f * node.config.scaleAcceleration.x * activeTime * activeTime);
                    finalScale.y = node.config.scale.y + (node.config.scaleVelocity.y * activeTime) + (0.5f * node.config.scaleAcceleration.y * activeTime * activeTime);
                    finalScale.z = node.config.scale.z + (node.config.scaleVelocity.z * activeTime) + (0.5f * node.config.scaleAcceleration.z * activeTime * activeTime);

                    break;
                case ScaleMode::UniformEasing:
                {
                    float t = activeTime / node.config.lifeTime;
                    float easeT = Mathf::EaseCurve(t, node.config.scaleUniformStartSpeed, node.config.scaleUniformEndSpeed);

                    finalScale.x = Mathf::Lerp(node.config.meshUniformStartScale, node.config.meshUniformEndScale, easeT);
                    finalScale.y = Mathf::Lerp(node.config.meshUniformStartScale, node.config.meshUniformEndScale, easeT);
                    finalScale.z = Mathf::Lerp(node.config.meshUniformStartScale, node.config.meshUniformEndScale, easeT);
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
                    this->angle.x + DirectX::XMConvertToRadians(node.config.rotation.x + (node.config.rotationVelocity.x * t) + (0.5f * node.config.rotationAcceleration.x * t * t)),
                    this->angle.y + DirectX::XMConvertToRadians(node.config.rotation.y + (node.config.rotationVelocity.y * t) + (0.5f * node.config.rotationAcceleration.y * t * t)),
                    this->angle.z + DirectX::XMConvertToRadians(node.config.rotation.z + (node.config.rotationVelocity.z * t) + (0.5f * node.config.rotationAcceleration.z * t * t))
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

                node.meshModel->UpdateTransform(worldTransform);

                if (node.config.lifeTime > maxDuration) maxDuration = node.config.lifeTime;
            }
        }
        // トレイル
        else if (node.config.type == RenderType::Trail)
        {
            if (effectTimer < node.config.delayTime) continue;

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

            if (node.config.lifeTime > maxDuration) maxDuration = node.config.lifeTime;
        }
    } 

    EmitParticles(elapsedTime, manager);

    // 寿命を進める
    if (maxDuration > 0.0f && effectTimer >= maxDuration)
    {
        bool isLoop = false;

        for (const auto& node : runTimeNodes)
        {
            if (node.config.type == RenderType::Particle && node.config.isLoop)
            {
                isLoop = true;
                break;
            }
        }

        if (isLoop)
        {
            effectTimer = 0.0f; // ループ
        }
        else
        {
            isPlaying = false; // 再生終了
        }
    }
}

// エフェクト描画
void CustomEffect::Render(RenderContext& rc)
{
    if (isPlaying)
    {
        for (const auto& node : runTimeNodes)
        {
            if (!node.config.visible) continue;

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
            if (!rNode.config.visible) continue;

            if (rNode.config.type == RenderType::Mesh)
            {
                float activeTime = effectTimer - rNode.config.delayTime;
                if (effectTimer >= rNode.config.delayTime && activeTime <= rNode.config.lifeTime)
                {
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
                        rNode.config.noiseUvOffset.x * effectTimer,
                        rNode.config.noiseUvOffset.y * effectTimer
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
                        finalColor = rNode.config.meshRandomColor;

                        break;
                    case ColorMode::Easing: // イージング
                    {
                        // 進行度を求める
                        float t = activeTime / rNode.config.lifeTime;

                        // イージング速度を求める
                        float easeT = Mathf::EaseCurve(t, rNode.config.colorStartSpeed, rNode.config.colorEndSpeed);

                        // Lerpを掛けて進行度にあ和得て色を変化させる
                        finalColor.x = Mathf::Lerp(rNode.config.meshStartColor.x, rNode.config.meshEndColor.x, easeT);
                        finalColor.y = Mathf::Lerp(rNode.config.meshStartColor.y, rNode.config.meshEndColor.y, easeT);
                        finalColor.z = Mathf::Lerp(rNode.config.meshStartColor.z, rNode.config.meshEndColor.z, easeT);
                        finalColor.w = Mathf::Lerp(rNode.config.meshStartColor.w, rNode.config.meshEndColor.w, easeT);

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
                        shader->Update(rc, mesh);
                        dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
                    }
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

// パーティクル放出
void CustomEffect::EmitParticles(float elapsedTime, std::unique_ptr<Compute_Particle>& manager)
{
    // 全ノードを順番にチェックして発生させる
    for (auto& node : runTimeNodes)
    {
        // パーティクルノード以外、または非表示ノードはスキップ
        if (node.config.type != RenderType::Particle) continue;
        if (!node.config.visible) continue;

        auto& pConfig = node.config;

        if (effectTimer < node.config.delayTime) continue;

        float activeTime = effectTimer - node.config.delayTime;
        if (node.config.duration > 0.0f && activeTime >= node.config.duration) continue;

        // ノード個別のタイマーを進める
        pConfig.spawnTimer += elapsedTime;

        float rand = Mathf::RandFloat();

        if (pConfig.spawnTimer >=pConfig.spawnInterval)
        {
           pConfig.spawnTimer -=pConfig.spawnInterval;

            // そのノードのテクスチャをGPUにセット（マルチテクスチャ対応！）
            if (manager && !pConfig.texturePath.empty())
            {
                manager->SetTexture(pConfig.texturePath, { (UINT)pConfig.splitCount[0], (UINT)pConfig.splitCount[1] });
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
                  pConfig.position.x + (pConfig.positionRange.x * Mathf::RandFloat()),
                  pConfig.position.y + (pConfig.positionRange.y * Mathf::RandFloat()),
                  pConfig.position.z + (pConfig.positionRange.z * Mathf::RandFloat()),
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
                    DirectX::XMConvertToRadians(pConfig.rotation.x + (pConfig.rotationRange.x * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotation.y + (pConfig.rotationRange.y * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotation.z + (pConfig.rotationRange.z * Mathf::RandFloat())),
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
                        std::max(0.0f, pConfig.scale.x + (pConfig.scaleRange.x * Mathf::RandFloat())),
                        std::max(0.0f, pConfig.scale.y + (pConfig.scaleRange.y * Mathf::RandFloat())),
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

                    data.scaleUniformStart = actualStart;
                    data.scaleUniformEnd = actualEnd;
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

                manager->Emit(data);
            }
        }
    }
}