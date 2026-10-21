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

    // 各インスタンスを作る
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

            // ノードでテクスチャが選ばれていれば差し替える(からの時はデフォルトのテクスチャを使う)
            if (!nodeConfig.texturePath.empty())
            {
                rNode.trail->SetTexture(nodeConfig.texturePath);
            }
        }
        else if (nodeConfig.type == RenderType::Ring)
        {
            rNode.ringMesh = CreateRingMesh(device, nodeConfig);
            if (!nodeConfig.texturePath.empty())
                rNode.shaderResourceView = TextureResourceManager::Instance().GetOrLoad(device, nodeConfig.texturePath);
            if (!nodeConfig.noisePath.empty())
                rNode.noiseShaderResourceView = TextureResourceManager::Instance().GetOrLoad(device, nodeConfig.noisePath);
        }
    }

    // 親のidを配列の番号に変換しておく
    for (auto& rNode : runTimeNodes)
    {
        rNode.parentIndex = config.FindNodeIndex(rNode.config.parentId);
    }

    // 親が先・子が後になる順番を作る（親が並んでいる順番から追加していく）
    updateOrder.clear();

    // 下の行の意味はrunTimeNodes.size()個の要素を持つstd::vector<bool>を作っていて、
    //全ての要素をfalseで初期化するという意味
    std::vector<bool>added(runTimeNodes.size(), false); // 各要素を追加済みかの印
    for (size_t pass = 0; pass < runTimeNodes.size(); ++pass)
    {
        for (size_t i = 0; i < runTimeNodes.size(); ++i)
        {
            if (added[i]) continue; // 追加済みならスキップする
            int p = runTimeNodes[i].parentIndex;
            if (p == -1 || added[p])
            {
                // 親が存在しないか、親が追加済みなら、処理順に追加して追加済みとして記録する
                updateOrder.push_back(static_cast<int>(i));
                added[i] = true;
            }
        }
    }

    // 万が一ループしていて並べられなかったノードは、親なし扱いで最後に積む
    for (size_t i = 0; i < runTimeNodes.size(); ++i)
    {
        if (!added[i])
        {
            runTimeNodes[i].parentIndex = -1;
            updateOrder.push_back(static_cast<int>(i));
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
            newNode.noisePath != oldNode.noisePath ||
            newNode.parentId != oldNode.parentId ||
            newNode.ringDivision != oldNode.ringDivision ||
            newNode.ringInner.x != oldNode.ringInner.x ||
            newNode.ringInner.y != oldNode.ringInner.y ||
            newNode.ringOuter.x != oldNode.ringOuter.x ||
            newNode.ringOuter.y != oldNode.ringOuter.y ||
            newNode.ringStartAngle != oldNode.ringStartAngle ||
            newNode.ringEndAngle != oldNode.ringEndAngle ||
            newNode.ringCenterRatio != oldNode.ringCenterRatio||
            newNode.ringInnerColor.x != oldNode.ringInnerColor.x ||
            newNode.ringInnerColor.y != oldNode.ringInnerColor.y ||
            newNode.ringInnerColor.z != oldNode.ringInnerColor.z ||
            newNode.ringInnerColor.w != oldNode.ringInnerColor.w ||
            newNode.ringCenterColor.x != oldNode.ringCenterColor.x ||
            newNode.ringCenterColor.y != oldNode.ringCenterColor.y ||
            newNode.ringCenterColor.z != oldNode.ringCenterColor.z ||
            newNode.ringCenterColor.w != oldNode.ringCenterColor.w ||
            newNode.ringOuterColor.x != oldNode.ringOuterColor.x ||
            newNode.ringOuterColor.y != oldNode.ringOuterColor.y ||
            newNode.ringOuterColor.z != oldNode.ringOuterColor.z ||
            newNode.ringOuterColor.w != oldNode.ringOuterColor.w 
            )
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

    // 1回目のループ（時間を進める）
    for (auto& node : runTimeNodes)
    {
        // UVスクロールの時間を進める
        node.currentUvOffset.x += node.config.uvScrollSpeed.x * elapsedTime;
        node.currentUvOffset.y += node.config.uvScrollSpeed.y * elapsedTime;

        node.currentUvOffset.x = fmodf(node.currentUvOffset.x, 1.0f);
        node.currentUvOffset.y = fmodf(node.currentUvOffset.y, 1.0f);

        if (!node.config.visible || node.finished) continue; // 非表示またはそのノードが終わったら飛ばす

        allFinished = false; // まだ終わってないノードある

        if (!node.started)
        {
            if (effectTimer < node.config.delayTime) continue; // 待機時間内ならスキップ

            // 待機時間が終わったらスタートする
            node.started = true;
            node.localTime = effectTimer - node.config.delayTime;
            BeginNodeCycle(node, true); // エフェクトの設定をする
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
                BeginNodeCycle(node, false); // 周期ごとに生成する
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
    } 

    DirectX::XMMATRIX effectMatrix = ComputeEffectMatrix();

    // ２回目のループ（行列を作る）
    for (int i : updateOrder)
    {
        auto& node = runTimeNodes[i];

        // 親のワールド行列(親なしならエフェクト全体の行列)
        DirectX::XMMATRIX parentWorld = effectMatrix;
        if (node.parentIndex != -1)
        {
            parentWorld = DirectX::XMLoadFloat4x4(&runTimeNodes[node.parentIndex].worldTransform);
        }

        // 非表示・まだ始まっていない・終わったノードも子のために行列は計算しておく
        DirectX::XMMATRIX world = ComputeLocalMatrix(node) * parentWorld;
        DirectX::XMStoreFloat4x4(&node.worldTransform, world);

        // トレイル
        if (node.config.type == RenderType::Trail && node.trail &&
            node.config.visible && node.started && !node.finished)
        {
            const auto& w = node.worldTransform;

            // 根本 = ノードの位置
            DirectX::XMFLOAT3 root = { w._41, w._42, w._43 };

            DirectX::XMFLOAT3 tip = {
                root.x + w._21 * node.config.width,
                root.y + w._22 * node.config.width,
                root.z + w._23 * node.config.width
            };

            if (node.trail)
            {
                node.trail->SetWidthScales(node.config.widthStart, node.config.widthMiddle, node.config.widthEnd);
                node.trail->SetBlendMode(node.config.blendMode);
                node.trail->SetColor(ComputeNodeColor(node));

                if (node.trailNeedsReset)
                {
                    node.trail->Reset(root, tip);
                    node.trailNeedsReset = false;
                }
                else
                {
                    node.trail->Update(root, tip, elapsedTime, node.config.trailLength);
                }
            }
        }
    }

    // パーティクルの計算を進める
    for (auto& node : runTimeNodes)
    {
        if (!node.particleManager) continue;

        bool local = (node.config.particleFollowMode == 1);
        node.particleManager->SetParent(node.worldTransform, local);
        // ブレンドモード設定
        node.particleManager->SetBlendMode(node.config.blendMode);

        // 相対位置モードは行列でついていくので移動差分は計算しない
        DirectX::XMFLOAT3 d = local ? DirectX::XMFLOAT3{ 0, 0, 0 } : delta;
        node.particleManager->Update(dc, d, elapsedTime);
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
        // メッシュとリング以外はここでは描かない
        const bool isMesh = (rNode.config.type == RenderType::Mesh && rNode.meshModel);
        const bool isRing = (rNode.config.type == RenderType::Ring && rNode.ringMesh);
        if (!isMesh && !isRing) continue;

        if (!rNode.config.visible || rNode.finished || !rNode.started) continue;

        float activeTime = rNode.localTime;

        DirectX::XMFLOAT2 noiseUvOffset = {
            rNode.config.noiseUvOffset.x * activeTime,
            rNode.config.noiseUvOffset.y * activeTime
        };

        shader->SetNoiseUvOffset(noiseUvOffset);
        shader->SetBlendMode(rNode.config.blendMode);
        shader->SetCurrentTextures(rNode.shaderResourceView.Get(), rNode.noiseShaderResourceView.Get());
        shader->SetDistortionPower(rNode.config.distortionPower);
        shader->SetColor(ComputeNodeColor(rNode));  // ← このノードの色を渡す
        shader->SetUvOffset(rNode.currentUvOffset); // ← UV スクロールを渡す
        shader->SetUseVertexColor(isRing);          // リングのときだけ頂点カラーを使う

        if (isMesh)
        {
            for (const auto& mesh : rNode.meshModel->GetMeshes())
            {
                DrawVFXMesh(dc, shader, rc, rNode, mesh);
            }
        }
        else
        {
            DrawVFXMesh(dc, shader, rc, rNode, *rNode.ringMesh);
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

        const bool local = (pConfig.particleFollowMode == 1);

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

                // 発生位置をワールド行列からとる
                // 発生位置：相対位置モードならノードの原点から、そうでなければワールド座標で
                DirectX::XMFLOAT3 origin = local ? DirectX::XMFLOAT3{ 0, 0, 0 }
                : DirectX::XMFLOAT3{ node.worldTransform._41, node.worldTransform._42, node.worldTransform._43 };

                // 位置
                data.position = {
                 origin.x + (pConfig.positionRange.x * Mathf::RandFloat()),
                 origin.y + (pConfig.positionRange.y * Mathf::RandFloat()),
                 origin.z + (pConfig.positionRange.z * Mathf::RandFloat()),
                   1.0f
                };
                // 初速度
                if (pConfig.emitShape == 1)
                {
                    // ランダムな向き　×　ランダムな速さ（全方向に飛ばす）
                    float z = Mathf::RandFloat(); // -1～1
                    float theta = Mathf::RandomRange(0.0f, DirectX::XM_2PI);
                    float r = sqrtf((std::max)(0.0f, 1.0f - z * z));
                    float speed = Mathf::RandomRange(pConfig.burstSpeedMin, pConfig.burstSpeedMax);
                    data.posVelocity = {
                        pConfig.positionVelocity.x + r * cosf(theta) * speed,
                        pConfig.positionVelocity.y + r * sinf(theta) * speed,
                        pConfig.positionVelocity.z + z * speed,
                        0.0f
                    };
                }
                else
                {
                    data.posVelocity = {
                        pConfig.positionVelocity.x + (pConfig.positionVelocityRange.x * Mathf::RandFloat()),
                        pConfig.positionVelocity.y + (pConfig.positionVelocityRange.y * Mathf::RandFloat()),
                        pConfig.positionVelocity.z + (pConfig.positionVelocityRange.z * Mathf::RandFloat()),
                         0.0f
                    };
                }
                // 加速度
                data.posAcceleration = {
                   pConfig.positionAcceleration.x + (pConfig.positionAccelerationRange.x * Mathf::RandFloat()),
                   pConfig.positionAcceleration.y + (pConfig.positionAccelerationRange.y * Mathf::RandFloat()),
                   pConfig.positionAcceleration.z + (pConfig.positionAccelerationRange.z * Mathf::RandFloat()),
                   pConfig.drag
                };

                DirectX::XMFLOAT3 baseAngle = local ? DirectX::XMFLOAT3{ 0, 0, 0 } : this->angle;

                // 回転
                data.rotation = {
                    DirectX::XMConvertToRadians(baseAngle.x + (pConfig.rotation.x + (pConfig.rotationRange.x * Mathf::RandFloat()))),
                    DirectX::XMConvertToRadians(baseAngle.y + (pConfig.rotation.y + (pConfig.rotationRange.y * Mathf::RandFloat()))),
                    DirectX::XMConvertToRadians(baseAngle.z + (pConfig.rotation.z + (pConfig.rotationRange.z * Mathf::RandFloat()))),
                    static_cast<float>(pConfig.billboardType)
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
                data.scale_param.z = pConfig.stretchScale; // 速度方向に伸ばす量


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

void CustomEffect::BeginNodeCycle(RuntimeNode& node, bool isFirstCycle)
{
    if (node.config.type == RenderType::Particle)
    {
        if (isFirstCycle)
        {
            node.spawnTimer = node.config.spawnInterval;
        }

        node.emitting = true;
    }

    if (node.config.type == RenderType::Mesh && node.config.scaleMode == ScaleMode::UniformEasing)
    {
        // 始点の拡大率を計算する
        node.meshUniformStartScale = node.config.scaleUniformStart + Mathf::RandomRange(-node.config.scaleUniformStartRandom, node.config.scaleUniformStartRandom); // 始点の中心 + 振れ幅
        node.meshUniformEndScale = node.config.scaleUniformEnd + Mathf::RandomRange(-node.config.scaleUniformEndRandom, node.config.scaleUniformEndRandom); // 終点の中心 + 振れ幅
    }

    const bool usesColor = (node.config.type == RenderType::Mesh ||
        node.config.type == RenderType::Trail ||
        node.config.type == RenderType::Ring);

    if (usesColor && node.config.colorMode == ColorMode::Random)
    {
        // 色をランダムで取得する
        node.meshRandomColor.x = Mathf::RandomRange(node.config.colorMin.x, node.config.colorMax.x);
        node.meshRandomColor.y = Mathf::RandomRange(node.config.colorMin.y, node.config.colorMax.y);
        node.meshRandomColor.z = Mathf::RandomRange(node.config.colorMin.z, node.config.colorMax.z);
        node.meshRandomColor.w = Mathf::RandomRange(node.config.colorMin.w, node.config.colorMax.w);
    }
    else if (usesColor && node.config.colorMode == ColorMode::Easing)
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

    if (node.config.type == RenderType::Trail && isFirstCycle)
    {
        node.trailNeedsReset = true;
    }
}

// ノード自身の行列（親やエフェクトは含めない）
DirectX::XMMATRIX CustomEffect::ComputeLocalMatrix(const RuntimeNode& node) const
{
    const auto& c = node.config;
    float t = node.localTime;

    // パーティクルは「どこから出すか」だけ。速度や大きさはパーティクル1つ1つの値なので行列にはいれない
    if (c.type == RenderType::Particle)
    {
        return DirectX::XMMatrixTranslation(c.position.x, c.position.y, c.position.z);
    }

    // 大きさ
    DirectX::XMFLOAT3 s = { 1, 1, 1 };
    if (c.type == RenderType::Mesh || c.type == RenderType::Ring)
    {
        switch (node.config.scaleMode)
        {
        case ScaleMode::Fixed:
            // 拡大時間経過で大きく/小さくする
            // scale = 初期拡大 + (拡大速度 * t) + (0.5 * 拡大加速度 * t^2)　物理の計算らしい
            s.x = c.scale.x + (c.scaleVelocity.x * t) + (0.5f * c.scaleAcceleration.x * t * t);
            s.y = c.scale.y + (c.scaleVelocity.y * t) + (0.5f * c.scaleAcceleration.y * t * t);
            s.z = c.scale.z + (c.scaleVelocity.z * t) + (0.5f * c.scaleAcceleration.z * t * t);

            break;
        case ScaleMode::UniformEasing:
        {
            float progress = (c.lifeTime > 0.0f) ? t / c.lifeTime : 0.0f;
            float easeT = Mathf::EaseCurve(progress, node.config.scaleUniformStartSpeed, node.config.scaleUniformEndSpeed);

            float uniform = Mathf::Lerp(node.meshUniformStartScale, node.meshUniformEndScale, easeT);
            s.x = uniform;
            s.y = uniform;
            s.z = uniform;
        }
        break;
        }
    }

    // マイナスにならないようにガード
    s.x = (std::max)(0.0f, s.x);
    s.y = (std::max)(0.0f, s.y);
    s.z = (std::max)(0.0f, s.z);

    // 回転（初期回転 ＋ 速度 + 加速度）
    DirectX::XMFLOAT3 r = {
        c.rotation.x + c.rotationVelocity.x * t + 0.5f * c.rotationAcceleration.x * t * t,
        c.rotation.y + c.rotationVelocity.y * t + 0.5f * c.rotationAcceleration.y * t * t,
        c.rotation.z + c.rotationVelocity.z * t + 0.5f * c.rotationAcceleration.z * t * t
    };

    // 位置（初期位置 ＋ 速度 + 加速度）
    DirectX::XMFLOAT3 p = {
        c.position.x  + (node.config.positionVelocity.x * t) + (0.5f * node.config.positionAcceleration.x * t * t),
        c.position.y  + (node.config.positionVelocity.y * t) + (0.5f * node.config.positionAcceleration.y * t * t),
        c.position.z  + (node.config.positionVelocity.z * t) + (0.5f * node.config.positionAcceleration.z * t * t)
    };

    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(s.x, s.y, s.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
        DirectX::XMConvertToRadians(r.x),
        DirectX::XMConvertToRadians(r.y),
        DirectX::XMConvertToRadians(r.z)
    );
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(p.x, p.y, p.z);

    return S * R * T;
}

// エフェクト全体の行列
DirectX::XMMATRIX CustomEffect::ComputeEffectMatrix() const
{
    DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
    DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
        DirectX::XMConvertToRadians(angle.x),
        DirectX::XMConvertToRadians(angle.y),
        DirectX::XMConvertToRadians(angle.z)
    );
    DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

    return S * R * T;
}

// リングの形をメッシュとして作る
std::shared_ptr<Model::Mesh> CustomEffect::CreateRingMesh(ID3D11Device* device, const NodeConfig& c)
{
    auto mesh = std::make_shared<Model::Mesh>();

    int division = (std::max)(3, c.ringDivision);
    float start = DirectX::XMConvertToRadians(c.ringStartAngle);
    float end = DirectX::XMConvertToRadians(c.ringEndAngle);

    // 中心は外輪と内輪の間（比率で決める）
    DirectX::XMFLOAT2 center = {
        c.ringOuter.x + (c.ringInner.x - c.ringOuter.x) * c.ringCenterRatio,
        c.ringOuter.y + (c.ringInner.y - c.ringOuter.y) * c.ringCenterRatio
    };

    // 頂点：分割ごとに「外側・内側」の2つ
    for (int i = 0; i <= division; ++i)
    {
        float u = static_cast<float>(i) / division; // 円周上の点を等間隔に配置するために0.0f～1.0に標準化する
        float angle = start + (end - start) * u; // 開始から終了までの線形補間
        float cs = cosf(angle);
        float sn = sinf(angle);

        Model::Vertex outer;
        outer.position = { cs * c.ringOuter.x, c.ringOuter.y, sn * c.ringOuter.x };
        outer.texcoord = { u, 0.0f };
        outer.tangent = c.ringOuterColor;          // ← 色を tangent に入れる
        mesh->vertices.push_back(outer);

        Model::Vertex mid;
        mid.position = { cs * center.x, center.y, sn * center.x };
        mid.texcoord = { u, c.ringCenterRatio };
        mid.tangent = c.ringCenterColor;
        mesh->vertices.push_back(mid);

        Model::Vertex inner;
        inner.position = { cs * c.ringInner.x, c.ringInner.y, sn * c.ringInner.x };
        inner.texcoord = { u, 1.0f };
        inner.tangent = c.ringInnerColor;
        mesh->vertices.push_back(inner);
    }

    // インデックス：1区画を三角形2枚で埋める（裏からも見えるように両面つくる）
    for (int i = 0; i < division; ++i)
    {
        uint32_t a = i * 3, b = (i + 1) * 3;   // 各角度の先頭（外輪）の番号
        for (int band = 0; band < 2; ++band)
        {
            uint32_t p0 = a + band, p1 = b + band;        // 外側の辺
            uint32_t q0 = a + band + 1, q1 = b + band + 1; // 内側の辺
            mesh->indices.insert(mesh->indices.end(), { p0, p1, q0,  q0, p1, q1 }); // 表
            mesh->indices.insert(mesh->indices.end(), { p0, q0, p1,  q0, q1, p1 }); // 裏
        }
    }

    // 頂点バッファ
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(sizeof(Model::Vertex) * mesh->vertices.size());
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = { mesh->vertices.data() };
    device->CreateBuffer(&vbDesc, &vbData, mesh->vertexBuffer.GetAddressOf());

    // インデックスバッファ
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh->indices.size());
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA ibData = { mesh->indices.data() };
    device->CreateBuffer(&ibDesc, &ibData, mesh->indexBuffer.GetAddressOf());

    return mesh;
}

void CustomEffect::DrawVFXMesh(ID3D11DeviceContext* dc, meshVFX* shader, RenderContext& rc, RuntimeNode& node, const Model::Mesh& mesh)
{
    // 頂点バッファ・インデックスバッファをセット
    UINT stride = sizeof(Model::Vertex);
    UINT offset = 0;
    dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
    dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 行列とパラメータを定数バッファに更新して描画コール
    shader->SetWorldTransForm(node.worldTransform);
    shader->Update(rc, mesh);
    dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
}

// 色とフェードをメッシュVFXに掛ける関数
DirectX::XMFLOAT4 CustomEffect::ComputeNodeColor(const RuntimeNode& node) const
{
    // フェードインの計算
    float alpha = 1.0f;
    float activeTime = node.localTime;
    if (node.config.IsFadeIn && node.config.fadeInTime > 0.0001f && activeTime <= node.config.fadeInTime)
    {
        // 現在の進行度を求める
        float t = activeTime / node.config.fadeInTime;
        alpha = Mathf::EaseCurve(t, node.config.fadeInStartSpeed, node.config.fadeInEndSpeed);
    }
    // フェードアウトの計算
    else if (node.config.IsFadeOut && node.config.fadeOutTime > 0.0001f)
    {
        // 進行度を求める
        float fadeStartTime = node.config.lifeTime - node.config.fadeOutTime;

        if (activeTime >= fadeStartTime)
        {
            float t = (activeTime - fadeStartTime) / node.config.fadeOutTime; // 0.0 ~ 1.0 に正規化
            alpha = 1.0f - Mathf::EaseCurve(t, node.config.fadeOutStartSpeed, node.config.fadeOutEndSpeed); // 1.0を引いて1.0～0.0のイージングにする
        }
    }

    // 0.0～1.0の範囲に収める
    alpha = Mathf::Clamp(alpha);


    // タイプごとに色の計算をする
    DirectX::XMFLOAT4 finalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    switch (node.config.colorMode)
    {
    case ColorMode::Fixed: // 固定色
        finalColor = node.config.color;

        break;
    case ColorMode::Random: // ランダム
        finalColor = node.meshRandomColor;

        break;
    case ColorMode::Easing: // イージング
    {
        // 進行度を求める
        float t = activeTime / node.config.lifeTime;

        // イージング速度を求める
        float easeT = Mathf::EaseCurve(t, node.config.colorStartSpeed, node.config.colorEndSpeed);

        finalColor.x = Mathf::Lerp(node.meshStartColor.x, node.meshEndColor.x, easeT);
        finalColor.y = Mathf::Lerp(node.meshStartColor.y, node.meshEndColor.y, easeT);
        finalColor.z = Mathf::Lerp(node.meshStartColor.z, node.meshEndColor.z, easeT);
        finalColor.w = Mathf::Lerp(node.meshStartColor.w, node.meshEndColor.w, easeT);

        break;
    }
    }

    finalColor.w *= alpha; // ← フェードを透明度に掛ける
    return finalColor;
}
