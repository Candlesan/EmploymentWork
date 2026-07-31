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

    // トレイルを読み込む
    trailRenderer = std::make_shared<Trail>();
    trailRenderer->Initialize();

    // モデルをロードする
    for (auto& node : config.nodes)
    {
        if (node.type == EffectNodeType::Mesh && !node.meshVfx.meshPath.empty())
        {
            vfxMeshModel = std::make_shared<Model>(device, node.meshVfx.meshPath.c_str());
        }
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

    // ① まず全ノードをチェックして、行列の計算や一番長い寿命（maxDuration）を調べる
    for (auto& node : config.nodes)
    {
        if (!node.visible) continue;

        // パーティクル
        if (node.type == EffectNodeType::Particle)
        {
            if (node.particle.duration > maxDuration) maxDuration = node.particle.duration;
        }
        // メッシュVFX
        else if (node.type == EffectNodeType::Mesh)
        {
            if (effectTimer < node.meshVfx.delayTime) continue;

            if (vfxMeshModel)
            {
                DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.meshVfx.scale.x, node.meshVfx.scale.y, node.meshVfx.scale.z);
                DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                    this->angle.x + DirectX::XMConvertToRadians(node.meshVfx.rotationSpeed.x * effectTimer),
                    this->angle.y + DirectX::XMConvertToRadians(node.meshVfx.rotationSpeed.y * effectTimer),
                    this->angle.z + DirectX::XMConvertToRadians(node.meshVfx.rotationSpeed.z * effectTimer));
                DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.meshVfx.position.x + this->position.x, node.meshVfx.position.y + this->position.y, node.meshVfx.position.z + this->position.z);

                DirectX::XMFLOAT4X4 worldTransform;
                DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);

                if (vfxMeshModel) vfxMeshModel->UpdateTransform(worldTransform);
                if (node.meshVfx.lifeTime > maxDuration) maxDuration = node.meshVfx.lifeTime;
            }
        }
        // トレイル
        else if (node.type == EffectNodeType::Trail)
        {
            if (effectTimer < node.trail.delayTime) continue;

            DirectX::XMFLOAT3 root = {
                this->position.x,
                this->position.y,
                this->position.z
            };

            DirectX::XMFLOAT3 tip = {
                root.x,
                root.y + node.trail.width,
                root.z
            };

            if (trailRenderer)
            {
                trailRenderer->SetWidthScales(node.trail.widthStart, node.trail.widthMiddle, node.trail.widthEnd);
                trailRenderer->Update(root, tip);
            }

            if (node.trail.lifeTime > maxDuration) maxDuration = node.trail.lifeTime;
        }
    } // 👈 ⭕ for 文はここで閉じる！

    // ② 全ノードの更新チェックが終わった後に1回だけパーティクルを生成する！
    EmitParticles(elapsedTime);

    // ③ 寿命判定も 1フレームに1回だけ行う！
    if (maxDuration > 0.0f && effectTimer >= maxDuration)
    {
        bool isLoop = false;

        for (const auto& node : config.nodes)
        {
            if (node.type == EffectNodeType::Particle && node.particle.isLoop)
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
        for (const auto& node : config.nodes)
        {
            if (!node.visible) continue;

            if (node.type == EffectNodeType::Trail && trailRenderer)
            {
                trailRenderer->Render(rc);
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
    if (isPlaying && vfxMeshModel)
    {
        for (const auto& node : config.nodes)
        {
            if (!node.visible) continue;

            if (node.type == EffectNodeType::Mesh)
            {
                float activeTime = effectTimer - node.meshVfx.delayTime;
                if (effectTimer >= node.meshVfx.delayTime && activeTime <= node.meshVfx.lifeTime)
                {
                    // ModelRendererから meshVFX シェーダーを取得
                    // ModelRenderer に GetShader 関数などを渡すか、直接セットする形を用意
                    auto* shader = dynamic_cast<meshVFX*>(modelRenderer->GetShader(ShaderId::MeshVFX));
                    if (shader)
                    {
                        // テクスチャをセット
                        shader->SetTexture(node.meshVfx.texturePath);
                        shader->SetNoiseTexture(node.meshVfx.noisePath);

                        shader->SetDistortionPower(node.meshVfx.distortionPower);

                        DirectX::XMFLOAT2 noiseUvOffset = {
                            node.meshVfx.noiseUvOffset.x * effectTimer,
                            node.meshVfx.noiseUvOffset.y * effectTimer
                        };
                        shader->SetNoiseUvOffset(noiseUvOffset);

                        shader->SetBlendMode(node.meshVfx.blendMode);

                        // 生存時間に応じてアルファ値の計算
                        float alphaRate = 1.0f - (effectTimer / node.meshVfx.lifeTime);

                        DirectX::XMFLOAT4 finalColor = node.meshVfx.color;
                        //finalColor.w *= alphaRate;

                        // 色をセット
                        shader->SetColor(finalColor);
                        // 経過時間 × スクロール速度 で UV オフセットを計算してセット！
                        DirectX::XMFLOAT2 uvOffset = {
                            node.meshVfx.uvScrollSpeed.x * effectTimer,
                            node.meshVfx.uvScrollSpeed.y * effectTimer
                        };
                        shader->SetUvOffset(uvOffset);
                    }

                    // MeshVFX シェーダーを指定して描画登録！
                    modelRenderer->Draw(ShaderId::MeshVFX, vfxMeshModel);
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
    for (auto& node : config.nodes)
    {
        // パーティクルノード以外、または非表示ノードはスキップ
        if (node.type != EffectNodeType::Particle || !node.visible) continue;

        auto& pConfig = node.particle;

        if (effectTimer < node.particle.delayTime) continue;

        float activeTime = effectTimer - node.particle.delayTime;
        if (node.particle.duration > 0.0f && activeTime >= node.particle.duration) continue;

        // ノード個別のタイマーを進める
        pConfig.spawnTimer += elapsedTime;

        float rand = Mathf::RandFloat();

        if (pConfig.spawnTimer >= pConfig.spawnInterval)
        {
            pConfig.spawnTimer -= pConfig.spawnInterval;

            // そのノードのテクスチャをGPUにセット（マルチテクスチャ対応！）
            if (particleManager && !pConfig.texturePath.empty())
            {
                particleManager->SetTexture(pConfig.texturePath, { (UINT)pConfig.splitCount[0], (UINT)pConfig.splitCount[1] });
            }

            // 生成数分 for 文で回す
            for (int i = 0; i < pConfig.emitCount; i++)
            {
                compute_particle_system::emit_particle_data data;
                data.parameter = { 0.0f, 0.0f, 0.0f, 0.0f };

                data.parameter.x = static_cast<float>(pConfig.spriteType);
                data.parameter.y = pConfig.lifeTime;
                data.parameter.z = pConfig.uvScrollSpeed.x;
                data.parameter.w = pConfig.uvScrollSpeed.y;

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