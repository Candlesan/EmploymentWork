#include "CustomEffect.h"
#include "System/Graphic/Graphics.h"
#include "System/Core/Mathf.h"
#include "System/Renderer/Shader/meshVFX/meshVFX.h"


// 初期化
void CustomEffect::Initialize(ID3D11Device* device, const EffectConfig& config)
{
    // エフェクトの設定を読み込む
    this->config = config;

    // パーティクルを読み込む
    particleManager = std::make_unique<Compute_Particle>(device);

    for (const auto& nodeConfig : config.nodes)
    {
        RuntimeNode rNode;
        rNode.config = nodeConfig;

        // メッシュノードなら、そのノードのパスでモデルを生成
        if (nodeConfig.type == RenderType::Mesh && !nodeConfig.meshPath.empty())
        {
            rNode.meshModel = std::make_unique<Model>(device, nodeConfig.meshPath.c_str());

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
}

// エフェクト再生
void CustomEffect::Play(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale)
{
	this->position = position;
	this->scale = scale;
	this->angle = angle;

	effectTimer = 0.0f; // タイマーを0にする
	isPlaying = true; // 再生中にする
}

// エフェクト更新
void CustomEffect::Update(float elapsedTime)
{
    if (!isPlaying) return;

    // パーティクルの更新
    if (particleManager)
    {
        particleManager->Update(Graphics::Instance().GetDeviceContext(), elapsedTime);
    }

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
                // 拡大（時間経過でのスケール変化 ＋ 拡大速度 ＋ 拡大加速度）
                DirectX::XMFLOAT3 currentScale = {
                    node.config.scale.x + (node.config.scaleVelocity.x * t) + (0.5f * node.config.scaleAcceleration.x * t * t),
                    node.config.scale.y + (node.config.scaleVelocity.y * t) + (0.5f * node.config.scaleAcceleration.y * t * t),
                    node.config.scale.z + (node.config.scaleVelocity.z * t) + (0.5f * node.config.scaleAcceleration.z * t * t)
                };
                // マイナス値にならないようにガード
                currentScale.x = (std::max)(0.0f, currentScale.x);
                currentScale.y = (std::max)(0.0f, currentScale.y);
                currentScale.z = (std::max)(0.0f, currentScale.z);

                DirectX::XMMATRIX S = DirectX::XMMatrixScaling(currentScale.x, currentScale.y, currentScale.z);

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

    EmitParticles(elapsedTime);

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

    // パーティクルの描画
    if (particleManager)
    {
        particleManager->Render(rc);
    }
}

void CustomEffect::RenderMeshVFX(ModelRenderer* modelRenderer)
{
    if (!isPlaying) return;

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
                    // ModelRendererから meshVFX シェーダーを取得
                    // ModelRenderer に GetShader 関数などを渡すか、直接セットする形を用意
                    auto* shader = dynamic_cast<meshVFX*>(modelRenderer->GetShader(ShaderId::MeshVFX));

                    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

                    if (shader)
                    {
                        // テクスチャをセット
                        shader->SetCurrentTextures(rNode.shaderResourceView.Get(), rNode.noiseShaderResourceView.Get());
                        shader->SetDistortionPower(rNode.config.distortionPower);

                        DirectX::XMFLOAT2 noiseUvOffset = {
                            rNode.config.noiseUvOffset.x * effectTimer,
                            rNode.config.noiseUvOffset.y * effectTimer
                        };
                        shader->SetNoiseUvOffset(noiseUvOffset);
                        shader->SetBlendMode(rNode.config.blendMode);
                        shader->SetColor(rNode.config.color);
                        shader->SetUvOffset(rNode.currentUvOffset);

                        // モデル内の各メッシュを即時描画
                        RenderContext rc;
                        rc.deviceContext = dc;
                        rc.renderState = Graphics::Instance().GetRenderState();
                        rc.camera = &CameraManager::Instance().GetMainCamera(); // カメラ参照

                        shader->Begin(rc);

                        for (const auto& mesh : rNode.meshModel->GetMeshes())
                        {
                            // 頂点バッファ・インデックスバッファをセット
                            UINT stride = sizeof(Model::Vertex);
                            UINT offset = 0;
                            dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
                            dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
                            dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                            // 行列とパラメータを定数バッファに更新して描画コール！
                            shader->Update(rc, mesh);
                            dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
                        }

                        shader->End(rc);
                    }

                    // MeshVFX シェーダーを指定して描画登録！
                    modelRenderer->Draw(ShaderId::MeshVFX, rNode.meshModel);
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
void CustomEffect::EmitParticles(float elapsedTime)
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
            if (particleManager && !pConfig.texturePath.empty())
            {
                particleManager->SetTexture(pConfig.texturePath, { (UINT)pConfig.splitCount[0], (UINT)pConfig.splitCount[1] });
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

                // 拡大
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

                data.color = pConfig.color;

                particleManager->Emit(data);
            }
        }
    }
}