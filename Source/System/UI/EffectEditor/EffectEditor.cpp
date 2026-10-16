#include "EffectEditor.h"
#include "System/UI/Dialog/Dialog.h"
#include "System/Graphic/Graphics.h"
#include "System/Shader/meshVFX/meshVFX.h"
#include "System/Core/Mathf.h"

#include <imgui.h>
#include <ctime>
#include <cstdlib>

// 初期化
void EffectEditor::Initialize(ID3D11Device* device)
{
    SyncRuntimeNodes();

    for (size_t i = 0; i < currentConfig.nodes.size(); ++i)
    {
        auto& rNode = runtimeNodes[i];

        // パーティクルマネージャーの初期化
        rNode.particleManager = std::make_unique<Compute_Particle>(device);
    }

    srand(static_cast<unsigned int>(time(nullptr)));
}

// GUI描画
void EffectEditor::DrawGUI()
{
	ImGui::Begin(u8"エフェクトエディター");


    if (selectedNodeIndex >= currentConfig.nodes.size())
    {
        selectedNodeIndex = 0;
    }

    // ツールバー
    DrawFileToolbar();
    ImGui::Separator();

    DrawNodeList();
    ImGui::Separator();

    // テスト再生ボタン
    if (ImGui::Button(u8"再生"))
    {
        PlayTestEffect(testSpawnPos);
    }

    // 選択中のノードを参照で受け取る
    auto& selectedNode = currentConfig.nodes[selectedNodeIndex];

    // ノード名の編集
    char nameBuffer[128];
    strcpy_s(nameBuffer, selectedNode.name.c_str());
    if (ImGui::InputText(u8"ノード名", nameBuffer, sizeof(nameBuffer)))
    {
        selectedNode.name = nameBuffer;
    }
    ImGui::Separator();

    DrawNodeInspector(selectedNode);
    ImGui::End();
}

// テスト再生
void EffectEditor::PlayTestEffect(const DirectX::XMFLOAT3& spawnPosition)
{
    testSpawnPos = spawnPosition;
    isPlaying = true;

    // 各種タイマーをリセット！
    effectTimer = 0.0f;

    for (int i = 0; i < currentConfig.nodes.size(); ++i)
    {
        if (i >= runtimeNodes.size()) break;

        auto& node = currentConfig.nodes[i];
        auto& rNode = runtimeNodes[i];

        if (node.type == RenderType::Particle)
        {
            rNode.spawnTimer = node.spawnInterval;
        }

        if (node.type == RenderType::Mesh && node.scaleMode == ScaleMode::UniformEasing)
        {
            // 始点の拡大率を計算する
            rNode.meshUniformStartScale = node.scaleUniformStart + Mathf::RandomRange(-node.scaleUniformStartRandom, node.scaleUniformStartRandom); // 始点の中心 + 振れ幅
            rNode.meshUniformEndScale = node.scaleUniformEnd + Mathf::RandomRange(-node.scaleUniformEndRandom, node.scaleUniformEndRandom); // 終点の中心 + 振れ幅
        }

        if (node.type == RenderType::Mesh && node.colorMode == ColorMode::Random)
        {
            // 色をランダムで取得する
            rNode.meshRandomColor.x = Mathf::RandomRange(node.colorMin.x, node.colorMax.x);
            rNode.meshRandomColor.y = Mathf::RandomRange(node.colorMin.y, node.colorMax.y);
            rNode.meshRandomColor.z = Mathf::RandomRange(node.colorMin.z, node.colorMax.z);
            rNode.meshRandomColor.w = Mathf::RandomRange(node.colorMin.w, node.colorMax.w);
        }
        else if (node.type == RenderType::Mesh && node.colorMode == ColorMode::Easing)
        {
            // 始点の色を指定した範囲でランダムに取得
            rNode.meshStartColor.x = Mathf::RandomRange(node.colorStartMin.x, node.colorStartMax.x);
            rNode.meshStartColor.y = Mathf::RandomRange(node.colorStartMin.y, node.colorStartMax.y);
            rNode.meshStartColor.z = Mathf::RandomRange(node.colorStartMin.z, node.colorStartMax.z);
            rNode.meshStartColor.w = Mathf::RandomRange(node.colorStartMin.w, node.colorStartMax.w);

            // 終点の色を指定した範囲でランダムに取得
            rNode.meshEndColor.x = Mathf::RandomRange(node.colorEndMin.x, node.colorEndMax.x);
            rNode.meshEndColor.y = Mathf::RandomRange(node.colorEndMin.y, node.colorEndMax.y);
            rNode.meshEndColor.z = Mathf::RandomRange(node.colorEndMin.z, node.colorEndMax.z);
            rNode.meshEndColor.w = Mathf::RandomRange(node.colorEndMin.w, node.colorEndMax.w);
        }
    }
}

// 更新処理
void EffectEditor::Update(ID3D11DeviceContext* dc, float elapsedTime)
{
    float maxDuration = 0.0f;

    // エフェクト全体の時間を進める
    effectTimer += elapsedTime;

    delta.x = testSpawnPos.x - prevSpawnPos.x;
    delta.y = testSpawnPos.y - prevSpawnPos.y;
    delta.z = testSpawnPos.z - prevSpawnPos.z;

    prevSpawnPos = testSpawnPos;

    for (auto& rNode : runtimeNodes)
    {
        if (rNode.particleManager)
        {
            // パーティクルの計算を進める
            rNode.particleManager->Update(dc, delta, elapsedTime);
        }
    }

    // 再生中じゃなければここで終了
    if (!isPlaying) return;

    for (size_t i = 0; i < currentConfig.nodes.size(); ++i)
    {
        auto& node = currentConfig.nodes[i];
        auto& rNode = runtimeNodes[i];

        if (!node.visible) continue;

        // UVスクロールの時間を進める
        rNode.currentUvOffset.x += node.uvScrollSpeed.x * elapsedTime;
        rNode.currentUvOffset.y += node.uvScrollSpeed.y * elapsedTime;
        rNode.currentUvOffset.x = fmodf(rNode.currentUvOffset.x, 1.0f);
        rNode.currentUvOffset.y = fmodf(rNode.currentUvOffset.y, 1.0f);

        float nodeTotalTime = 0.0f;
        if (node.type == RenderType::Particle)
        {
            // パーティクルの場合：発生遅延 ＋ 存続時間 ＋ 寿命
            nodeTotalTime = node.delayTime + node.duration + node.lifeTime;
        }
        else
        {
            // メッシュ・トレイルの場合：発生遅延 ＋ 寿命
            nodeTotalTime = node.delayTime + node.lifeTime;
        }

        // ここで全ノード共通で maxDuration を更新する！
        if (nodeTotalTime > maxDuration)
        {
            maxDuration = nodeTotalTime;
        }

        // メッシュの処理
        if (node.type == RenderType::Mesh)
        {
            if (effectTimer < node.delayTime) continue;

            if (rNode.meshModel)
            {
                float activeTime = effectTimer - node.delayTime;

                DirectX::XMFLOAT3 finalScale = { 1, 1, 1 };
                switch (node.scaleMode)
                {
                case ScaleMode::Fixed:
                    // 拡大時間経過で大きく/小さくする
                    // scale = 初期拡大 + (拡大速度 * t) + (0.5 * 拡大加速度 * t^2)　物理の計算らしい
                    finalScale.x = node.scale.x + (node.scaleVelocity.x * activeTime) + (0.5f * node.scaleAcceleration.x * activeTime * activeTime);
                    finalScale.y = node.scale.y + (node.scaleVelocity.y * activeTime) + (0.5f * node.scaleAcceleration.y * activeTime * activeTime);
                    finalScale.z = node.scale.z + (node.scaleVelocity.z * activeTime) + (0.5f * node.scaleAcceleration.z * activeTime * activeTime);

                    break;
                case ScaleMode::UniformEasing:
                {
                    float t = activeTime / node.lifeTime;
                    float easeT = Mathf::EaseCurve(t, node.scaleUniformStartSpeed, node.scaleUniformEndSpeed);

                    finalScale.x = Mathf::Lerp(rNode.meshUniformStartScale, rNode.meshUniformEndScale, easeT);
                    finalScale.y = Mathf::Lerp(rNode.meshUniformStartScale, rNode.meshUniformEndScale, easeT);
                    finalScale.z = Mathf::Lerp(rNode.meshUniformStartScale, rNode.meshUniformEndScale, easeT);
                }
                    break;
                }


                // マイナスにならないようにガード
                finalScale.x = (std::max)(0.0f, finalScale.x);
                finalScale.y = (std::max)(0.0f, finalScale.y);
                finalScale.z = (std::max)(0.0f, finalScale.z);

                DirectX::XMMATRIX S = DirectX::XMMatrixScaling(finalScale.x, finalScale.y, finalScale.z);

                // 回転
                DirectX::XMFLOAT3 currentRot = {
                            node.rotation.x + (node.rotationVelocity.x * activeTime) + (0.5f * node.rotationAcceleration.x * activeTime * activeTime),
                            node.rotation.y + (node.rotationVelocity.y * activeTime) + (0.5f * node.rotationAcceleration.y * activeTime * activeTime),
                            node.rotation.z + (node.rotationVelocity.z * activeTime) + (0.5f * node.rotationAcceleration.z * activeTime * activeTime)
                };
                DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(currentRot.x),
                    DirectX::XMConvertToRadians(currentRot.y),
                    DirectX::XMConvertToRadians(currentRot.z)
                );

                // 位置
                DirectX::XMFLOAT3 currentPos = {
                            node.position.x + (node.positionVelocity.x * activeTime) + (0.5f * node.positionAcceleration.x * activeTime * activeTime),
                            node.position.y + (node.positionVelocity.y * activeTime) + (0.5f * node.positionAcceleration.y * activeTime * activeTime),
                            node.position.z + (node.positionVelocity.z * activeTime) + (0.5f * node.positionAcceleration.z * activeTime * activeTime)
                };

                DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
                    testSpawnPos.x + currentPos.x,
                    testSpawnPos.y + currentPos.y,
                    testSpawnPos.z + currentPos.z
                );

                // トランスフォーム行列を合成してモデルに適用
                DirectX::XMFLOAT4X4 worldTransform;
                DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);
                rNode.worldTransform = worldTransform;
            }
        }
        // トレイルの処理
        else if (node.type == RenderType::Trail)
        {
            if (effectTimer < node.delayTime) continue;

            // エフェクト時間経過に合わせて、テスト位置を中心に円を描くように座標を計算する
            float speed = 5.0f; // 速度
            float radius = 1.5f; // 円の半径
            float currentAngle = effectTimer * speed;

            // 根本と先端を求める
            DirectX::XMFLOAT3 root = {
            testSpawnPos.x + cosf(currentAngle) * radius,
            testSpawnPos.y,
            testSpawnPos.z + sinf(currentAngle) * radius
            };

            DirectX::XMFLOAT3 tip = {
                root.x,
                root.y + node.width, // 幅（太さ）分だけ上にする
                root.z
            };

            // Trailクラスに過去座標として記録させる
            if (rNode.trail)
            {
                rNode.trail->SetWidthScales(node.widthStart, node.widthMiddle, node.widthEnd);
                rNode.trail->Update(root, tip);
            }
        }

    }

    // EmitParticle内で全ノードを回して生成する
    EmitParticles(elapsedTime);

    if (maxDuration > 0.0f && effectTimer >= maxDuration)
    {
        // 選択中またはどれか一つでもループがオンになっているか
        bool isLoop = false;
        for (const auto& node : currentConfig.nodes)
        {
            if (node.type == RenderType::Particle && node.isLoop)
            {
                isLoop = true;
                break;
            }
            else if (node.type == RenderType::Mesh && node.isLoop)
            {
                isLoop = true;
                break;
            }
        }

        if (isLoop)
        {
            effectTimer = 0.0f; // ループリセット
        }
        else
        {
            isPlaying = false;  // 再生終了
        }
    }

}

// 描画処理
void EffectEditor::Render(const RenderContext& rc, ModelRenderer* modelRenderer)
{
    if (isPlaying)
    {
        for (size_t i = 0; i < currentConfig.nodes.size(); ++i)
        {
            auto& node = currentConfig.nodes[i];
            if (i >= runtimeNodes.size()) break;
            auto& rNode = runtimeNodes[i];

            if (!node.visible) continue;

            // パーティクルの描画
            if (rNode.particleManager)
            {
                rNode.particleManager->Render(rc);
            }

            if (node.type == RenderType::Trail && rNode.trail)
            {
                rNode.trail->Render(rc);
            }
        }
    }
}

// メッシュVFX描画
void EffectEditor::RenderMeshVFX(ModelRenderer* modelRenderer)
{
    if (!isPlaying) return;

    // シェーダーの開始処理（パイプライン設定）
    auto* shader = dynamic_cast<meshVFX*>(modelRenderer->GetShader(ShaderId::MeshVFX));
    if (!shader) return;

    ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

    for (size_t i = 0; i < currentConfig.nodes.size(); ++i)
    {
        auto& node = currentConfig.nodes[i];
        if (i >= runtimeNodes.size()) break;
        auto& rNode = runtimeNodes[i];

        // 表示されてなくて描画方式がmesh出ないときは最初から
        if (!node.visible || node.type != RenderType::Mesh) continue;

        // 
        if (effectTimer < node.delayTime) continue;
        float meshActiveTime = effectTimer - node.delayTime;

        float alpha = 1.0f;

        if (rNode.meshModel && meshActiveTime <= node.lifeTime)
        {
            // このノード専用のパラメータをシェーダーにセット
            shader->SetCurrentTextures(rNode.shaderResourceView.Get(), rNode.noiseShaderResourceView.Get());
            shader->SetDistortionPower(node.distortionPower);

            // フェードインの計算
            if (node.IsFadeIn && node.fadeInTime > 0.0001f && meshActiveTime <= node.fadeInTime)
            {
                // 現在の進行度を求める
                float t = meshActiveTime / node.fadeInTime;
                alpha = Mathf::EaseCurve(t, node.fadeInStartSpeed, node.fadeInEndSpeed);
            }
            // フェードアウトの計算
            else if (node.IsFadeOut && node.fadeOutTime > 0.0001f)
            {
                // 進行度を求める
                float fadeStartTime = node.lifeTime - node.fadeOutTime;

                if (meshActiveTime >= fadeStartTime)
                {
                    float t = (meshActiveTime - fadeStartTime) / node.fadeOutTime; // 0.0 ~ 1.0 に正規化
                    alpha = 1.0f - Mathf::EaseCurve(t, node.fadeOutStartSpeed, node.fadeOutEndSpeed); // 1.0を引いて1.0～0.0のイージングにする
                }
            }

            // 0.0～1.0の範囲に収める
            alpha = Mathf::Clamp(alpha);

            // ノイズテクスチャの移動値を計算する
            DirectX::XMFLOAT2 noiseUvOffset = {
                node.noiseUvOffset.x * meshActiveTime,
                node.noiseUvOffset.y * meshActiveTime
            };
            shader->SetNoiseUvOffset(noiseUvOffset);
            shader->SetBlendMode(node.blendMode);

            // タイプごとに色の計算をする
            DirectX::XMFLOAT4 finalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
            switch (node.colorMode)
            {
            case ColorMode::Fixed: // 固定色
                finalColor = node.color;

                break;
            case ColorMode::Random: // ランダム
                finalColor = rNode.meshRandomColor;

                break;
            case ColorMode::Easing: // イージング
            {
                // 進行度を求める
                float t = meshActiveTime / node.lifeTime;

                // イージング速度を求める
                float easeT = Mathf::EaseCurve(t, node.colorStartSpeed, node.colorEndSpeed);

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
                shader->SetWorldTransForm(rNode.worldTransform);
                shader->Update(rc, mesh);
                dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
            }

            shader->End(rc);
        }
    }
}

// UIの描画パーツ
void EffectEditor::DrawNodeInspector(NodeConfig& node)
{
    ImGui::Checkbox(u8"このノードを表示する##VisibleCheck", &node.visible);
    ImGui::Separator();

    if (ImGui::BeginTabBar(u8"ParticleTabs"))
    {
        //　共通タブ
        if (ImGui::BeginTabItem(u8"共通"))
        {
            ImGui::DragInt(u8"生成数##Particle", &node.emitCount, 1);
            ImGui::Separator();

            ImGui::Separator();

            ImGui::DragFloat(u8"寿命##Particle", &node.lifeTime, 0.05f, 0.1f, 10.0f);
            ImGui::Separator();

            ImGui::Checkbox(u8"ループ再生##Particle", &node.isLoop);
            ImGui::Separator();

            ImGui::DragFloat(u8"生成間隔(秒)##Particle", &node.spawnInterval, 0.01f, 0.01f, 5.0f, "%.2fs");
            ImGui::DragFloat(u8"存続時間(秒)##Particle", &node.duration, 0.1f, 0.0f, 60.0f, u8"%.1fs (0=無限)");
            ImGui::DragFloat(u8"発生遅延(秒)##Particle", &node.delayTime, 0.05f, 0.0f, 10.0f, "%.2fs");

            ImGui::EndTabItem();
        }

        // 位置タブ
        if (ImGui::BeginTabItem(u8"位置"))
        {
            ImGui::DragFloat3(u8"初期位置##Pos", &node.position.x, 0.1f);
            ImGui::DragFloat3(u8"位置ばらつき##Pos", &node.positionRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"初速度##Pos", &node.positionVelocity.x, 0.1f);
            ImGui::DragFloat3(u8"初速度ばらつき##Pos", &node.positionVelocityRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"加速度##Pos", &node.positionAcceleration.x, 0.1f);
            ImGui::DragFloat3(u8"加速度ばらつき##Pos", &node.positionAccelerationRange.x, 0.1f);

            ImGui::EndTabItem();
        }

        // 回転タブ
        if (ImGui::BeginTabItem(u8"回転"))
        {
            ImGui::DragFloat3(u8"初期回転##Rot", &node.rotation.x, 0.1f);
            ImGui::DragFloat3(u8"回転ばらつき##Rot", &node.rotationRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"角速度##Rot", &node.rotationVelocity.x, 0.1f);
            ImGui::DragFloat3(u8"角速度ばらつき##Rot", &node.rotationVelocityRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"回転加速度##Rot", &node.rotationAcceleration.x, 0.1f);
            ImGui::DragFloat3(u8"回転加速度ばらつき##Rot", &node.rotationAccelerationRange.x, 0.1f);

            ImGui::EndTabItem();
        }

        // 拡大（スケール）タブ
        if (ImGui::BeginTabItem(u8"拡大"))
        {
            const char* scaleModeItems[] = { u8"拡大率・速度・加速度", u8"単一イージング"};
            int scaleMode = static_cast<int>(node.scaleMode);
            if (ImGui::Combo(u8"##Scale", &scaleMode, scaleModeItems, IM_ARRAYSIZE(scaleModeItems)))
            {
                node.scaleMode = static_cast<ScaleMode>(scaleMode);
            }
            ImGui::Separator();

            switch (node.scaleMode)
            {
            case ScaleMode::Fixed:
                ImGui::Text(u8"拡大率");
                ImGui::DragFloat3(u8"中心##scale", &node.scale.x, 0.01f);
                ImGui::DragFloat3(u8"振幅##scale", &node.scaleRange.x, 0.01f);
                ImGui::Separator();

                ImGui::Text(u8"拡大速度");
                ImGui::DragFloat3(u8"中心##scaleVelocity", &node.scaleVelocity.x, 0.01f);
                ImGui::DragFloat3(u8"振幅##scaleVelocity", &node.scaleVelocityRange.x, 0.01f);
                ImGui::Separator();

                ImGui::Text(u8"拡大加速度");
                ImGui::DragFloat3(u8"拡大加速度##scaleAcceleration", &node.scaleAcceleration.x, 0.01f);
                ImGui::DragFloat3(u8"拡大加速度ばらつき##scaleAcceleration", &node.scaleAccelerationRange.x, 0.01f);
                ImGui::Separator();

                break;

            case ScaleMode::UniformEasing:
                ImGui::Text(u8"始点");
                ImGui::DragFloat(u8"中心##scaleUniformStart", &node.scaleUniformStart, 0.01f);
                ImGui::DragFloat(u8"振幅##scaleUniformStart", &node.scaleUniformStartRandom, 0.01f);
                ImGui::Separator();

                ImGui::Text(u8"終点");
                ImGui::DragFloat(u8"中心##scaleUniformEnd", &node.scaleUniformEnd, 0.01f);
                ImGui::DragFloat(u8"振幅##scaleUniformEnd", &node.scaleUniformEndRandom, 0.01f);
                ImGui::Separator();

                const char* speedItem[] = { u8"低速3", u8"低速2",  u8"低速1",  u8"等速",  u8"高速1",  u8"高速2",  u8"高速3" };
                ImGui::Combo(u8"始点速度##ColorSpeed", &node.scaleUniformStartSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();

                ImGui::Combo(u8"終点速度##ColorSpeed", &node.scaleUniformEndSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();

                break;
            }

            ImGui::EndTabItem();
        }

        // 描画共通タブ
        if (ImGui::BeginTabItem(u8"描画共通"))
        {
            // テクスチャ選択
            if (ImGui::Button(u8"読込##Particle"))
            {
                char filePath[MAX_PATH] = { 0 };
                HWND hwnd = Graphics::Instance().GetWindowHandle();
                if (Dialog::OpenFileName(filePath, MAX_PATH,
                    "PNG File\0*.png\0DDS File\0*.dds\0TGA File\0*.tga\0All Files\0*.*\0\0",
                    "テクスチャを選択",
                    hwnd)
                    == DialogResult::OK)
                {
                    std::string pathStr = filePath;

                    // Dataという文字を探してそれ以降のパスを切り取る
                    size_t pos = pathStr.find("Data");
                    if (pos != std::string::npos)
                    {
                        pathStr = pathStr.substr(pos);
                    }

                    currentConfig.nodes[selectedNodeIndex].texturePath = pathStr;

                    if (runtimeNodes[selectedNodeIndex].particleManager)
                    {
                        runtimeNodes[selectedNodeIndex].particleManager->SetTexture(
                            pathStr,
                            { (UINT)currentConfig.nodes[selectedNodeIndex].splitCount[0], (UINT)currentConfig.nodes[selectedNodeIndex].splitCount[1] }
                        );
                    }

                    SyncRuntimeNodes();
                }
            }
            ImGui::SameLine();

            ImGui::Text(u8": % s", node.texturePath.c_str());

            // テクスチャ解除
            if (!node.texturePath.empty())
            {
                if (ImGui::Button(u8"テクスチャ解除##TextuerClear"))
                {
                    node.texturePath = "";

                    if (runtimeNodes[selectedNodeIndex].particleManager)
                    {
                        runtimeNodes[selectedNodeIndex].particleManager->SetTexture("", { 1, 1 });
                    }
                }
            }

            ImGui::Separator();


            ImGui::Checkbox(u8"ビルボード##Particle", &node.isBillboard);
            ImGui::Separator();

            const char* blendItems[] = { u8"通常半透明 (Alpha)", u8"加算 (Additive)" };
            ImGui::Combo(u8"ブレンドモード##Mesh", &node.blendMode, blendItems, IM_ARRAYSIZE(blendItems));
            ImGui::Separator();

            const char* speedItem[] = { u8"低速3", u8"低速2",  u8"低速1",  u8"等速",  u8"高速1",  u8"高速2",  u8"高速3" };
            ImGui::Checkbox(u8"フェードイン", &node.IsFadeIn);
            if (node.IsFadeIn)
            {
                node.IsFadeIn = true;
                ImGui::DragFloat(u8"終了時間##FadeIn", &node.fadeInTime, 0.1f);
                ImGui::Separator();

                ImGui::Combo(u8"始点速度##FadeIn", &node.fadeInStartSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();

                ImGui::Combo(u8"終点速度##FadeIn", &node.fadeInEndSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();
            }
            else
            {
                node.IsFadeIn = false;
            }

            ImGui::Checkbox(u8"フェードアウト", &node.IsFadeOut);
            if (node.IsFadeOut)
            {
                node.IsFadeOut = true;
                ImGui::DragFloat(u8"終了時間##FadeOut", &node.fadeOutTime, 0.1f);
                ImGui::Separator();

                ImGui::Combo(u8"始点速度##FadeOut", &node.fadeOutStartSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();

                ImGui::Combo(u8"終点速度##FadeOut", &node.fadeOutEndSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();
            }
            else
            {
                node.IsFadeOut = false;
            }


            if (ImGui::InputInt2(u8"画像分割数(X,Y)##Particle", node.splitCount))
            {
                // 分割数の数値をいじった瞬間にGPU側の設定も更新する
                if (runtimeNodes[selectedNodeIndex].particleManager && !node.texturePath.empty()) {
                    runtimeNodes[selectedNodeIndex].particleManager->SetTexture(node.texturePath, { (UINT)node.splitCount[0], (UINT)node.splitCount[1] });
                }
            }

            ImGui::SliderInt(u8"コマ番号##Particle", &node.spriteType, 0, 15);
            ImGui::DragFloat2(u8"UVスクロール速度(X,Y)##Particle", &node.uvScrollSpeed.x, 0.01f, -10.0f, 10.0f, "%.2f");

            ImGui::EndTabItem();
        }

        // カラータブ
        if (ImGui::BeginTabItem(u8"描画"))
        {
            const char* renderTypeItems[] = {
                u8"スプライト",
                u8"モデル",
                u8"軌跡",
                u8"無し"
            };
            int currentType = static_cast<int>(node.type);

            if (ImGui::Combo(u8"描画方法##RenderTypeCombo", &currentType, renderTypeItems, IM_ARRAYSIZE(renderTypeItems)))
            {
                node.type = static_cast<RenderType>(currentType);
                SyncRuntimeNodes();
            }
            ImGui::Separator();

            const char* colortype[] = { u8"固定", u8"ランダム", u8"イージング" };
            int colorMode = static_cast<int>(node.colorMode);
            if (ImGui::Combo(u8"##Color", &colorMode, colortype, IM_ARRAYSIZE(colortype)))
            {
                node.colorMode = static_cast<ColorMode>(colorMode);
            }
            switch (node.colorMode)
            {
            case ColorMode::Fixed:
                ImGui::ColorEdit4(u8"", &node.color.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();

                break;
            case ColorMode::Random:
                ImGui::ColorEdit4(u8"最小##ColorRandom", &node.colorMin.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();
                ImGui::ColorEdit4(u8"最大##ColorRandom", &node.colorMax.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();

                break;
            case ColorMode::Easing:
            {
                ImGui::Text(u8"始点");
                ImGui::ColorEdit4(u8"最小##ColorStartEaseing", &node.colorStartMin.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();
                ImGui::ColorEdit4(u8"最大##ColorStartEaseing", &node.colorStartMax.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();

                ImGui::Text(u8"終点");
                ImGui::ColorEdit4(u8"最小##ColorEndEaseing", &node.colorEndMin.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();
                ImGui::ColorEdit4(u8"最大##ColorEndEaseing", &node.colorEndMax.x, ImGuiColorEditFlags_Uint8);
                ImGui::Separator();

                const char* speedItem[] = { u8"低速3", u8"低速2",  u8"低速1",  u8"等速",  u8"高速1",  u8"高速2",  u8"高速3" };
                ImGui::Combo(u8"始点速度##ColorSpeed", &node.colorStartSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();

                ImGui::Combo(u8"終点速度##ColorSpeed", &node.colorEndSpeed, speedItem, IM_ARRAYSIZE(speedItem));
                ImGui::Separator();

                break;
            }
            }

            if (node.type == RenderType::Mesh)
            {
                // モデル読込 (.gltf / .glb)
                if (ImGui::Button(u8"モデル読込##Mesh"))
                {
                    char filePath[MAX_PATH] = { 0 };
                    HWND hwnd = Graphics::Instance().GetWindowHandle();
                    if (Dialog::OpenFileName(filePath, MAX_PATH, "GLTF/GLB File\0*.gltf;*.glb\0\0", "モデル選択", hwnd) == DialogResult::OK)
                    {
                        std::string pathStr = filePath;
                        size_t pos = pathStr.find("Data");
                        if (pos != std::string::npos) pathStr = pathStr.substr(pos);

                        node.meshPath = pathStr;
                        ID3D11Device* device = Graphics::Instance().GetDevice();

                        SyncRuntimeNodes();

                        if (selectedNodeIndex < runtimeNodes.size())
                        {
                            runtimeNodes[selectedNodeIndex].meshModel = std::make_shared<Model>(device, pathStr.c_str());
                        }
                    }
                }
                ImGui::SameLine();

                ImGui::Text(u8": %s", node.meshPath.c_str());
                ImGui::Separator();

                if (!node.meshPath.empty())
                {
                    if (ImGui::Button(u8"モデル解除##MeshClear"))
                    {
                        // 設定データのパスをクリア
                        node.meshPath = "";

                        // 実際に保持している 3Dモデルポインタを nullptr で解放
                        SyncRuntimeNodes(); // 安全のためサイズ同期
                        if (selectedNodeIndex < runtimeNodes.size())
                        {
                            runtimeNodes[selectedNodeIndex].meshModel = nullptr;
                        }
                    }
                }
                ImGui::Separator();

                // テクスチャ ＆ ノイズテクスチャ
                if (ImGui::Button(u8"ノイズテクスチャ読込##Mesh"))
                {
                    char filePath[MAX_PATH] = { 0 };
                    HWND hwnd = Graphics::Instance().GetWindowHandle();
                    if (Dialog::OpenFileName(filePath, MAX_PATH,
                        "PNG File\0*.png\0DDS File\0*.dds\0TGA File\0*.tga\0All Files\0*.*\0\0",
                        "テクスチャ選択",
                        hwnd) == DialogResult::OK)
                    {
                        std::string pathStr = filePath;
                        size_t pos = pathStr.find("Data");
                        if (pos != std::string::npos) pathStr = pathStr.substr(pos);

                        node.noisePath = pathStr;
                    }
                }
                ImGui::SameLine();

                ImGui::Text(u8": %s", node.noisePath.c_str());
                ImGui::Separator();

                if (!node.noisePath.empty())
                {
                    if (ImGui::Button(u8"ノイズテクスチャ解除##NoiseClear"))
                    {
                        node.noisePath = "";        // パスをクリア
                        node.distortionPower = 0.0f; // 歪みの強さも0に戻しておく
                    }
                }
                ImGui::Separator();

                ImGui::DragFloat(u8"ノイズの強さ", &node.distortionPower, 0.05f, 0.0f, 10.0f);
                ImGui::DragFloat2(u8"ノイズUVスクロール", &node.noiseUvOffset.x, 0.01f, -10.0f, 10.0f);
            }
            else if (node.type == RenderType::Trail)
            {
                // トレイル用テクスチャ読込 ＆ 幅設定
                ImGui::DragFloat(u8"幅(太さ)", &node.width, 0.1f);
                ImGui::Text(u8"【幅スケール (形状)】");
                ImGui::DragFloat(u8"先端(頭)", &node.widthStart, 0.01f, 0.0f, 5.0f);
                ImGui::DragFloat(u8"中央", &node.widthMiddle, 0.01f, 0.0f, 5.0f);
                ImGui::DragFloat(u8"末端(尻)", &node.widthEnd, 0.01f, 0.0f, 5.0f);
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}
void EffectEditor::DrawFileToolbar()
{
    // Save ボタン
    if (ImGui::Button(u8"Save"))
    {
        char filepath[MAX_PATH] = { 0 };
        HWND hwnd = Graphics::Instance().GetWindowHandle();
        if (Dialog::SaveFileName(filepath, MAX_PATH,
            "JSON File\0*.json\0\0",	// フィルター
            "名前を付けて保存",	// タイトル
            "json",// 拡張子
            hwnd) == DialogResult::OK)
        {
            currentConfig.Save(filepath);
        }
    }
    ImGui::SameLine();

    // Load ボタン
    if (ImGui::Button(u8"Load"))
    {
        char filepath[MAX_PATH] = { 0 };
        HWND hwnd = Graphics::Instance().GetWindowHandle();
        if (Dialog::OpenFileName(filepath, MAX_PATH,
            "JSON File\0*.json\0\0",
            "ファイルを開く",
            hwnd) == DialogResult::OK)
        {
            runtimeNodes.clear(); // 読み込む前にruntimeNodesを空にする

            currentConfig.Load(filepath);

            SyncRuntimeNodes();

            // 読み込んだノード数を超えていたら安全な範囲に戻す
            if (selectedNodeIndex >= static_cast<int>(currentConfig.nodes.size()))
            {
                selectedNodeIndex = 0;
            }

            if (runtimeNodes[selectedNodeIndex].particleManager && !currentConfig.nodes[selectedNodeIndex].texturePath.empty())
            {
                runtimeNodes[selectedNodeIndex].particleManager->SetTexture(
                    currentConfig.nodes[selectedNodeIndex].texturePath,
                    { (UINT)currentConfig.nodes[selectedNodeIndex].splitCount[0], (UINT)currentConfig.nodes[selectedNodeIndex].splitCount[1] }
                );
            }

            // メッシュパスが有効ならモデルもリロード
            if (!currentConfig.nodes[selectedNodeIndex].meshPath.empty())
            {
                ID3D11Device* device = Graphics::Instance().GetDevice();
                runtimeNodes[selectedNodeIndex].meshModel = std::make_shared<Model>(device, currentConfig.nodes[selectedNodeIndex].meshPath.c_str());
            }
        }
    }
}
void EffectEditor::DrawNodeList()
{
    ImGui::Text(u8"【ノード一覧】");

    if (ImGui::Button(u8"+ ノード追加"))
    {
        NodeConfig newNode;
        newNode.name = "Node " + std::to_string(currentConfig.nodes.size() + 1);
        newNode.type = RenderType::Particle;
        currentConfig.nodes.push_back(newNode);
        selectedNodeIndex = static_cast<int>(currentConfig.nodes.size() - 1);

        SyncRuntimeNodes();
    }
    ImGui::SameLine();

    // ノードが1個しかない時は削除できないようにする
    if (currentConfig.nodes.size() > 1)
    {
        if (ImGui::Button(u8"- 選択ノード削除"))
        {
            currentConfig.nodes.erase(currentConfig.nodes.begin() + selectedNodeIndex);
            runtimeNodes.erase(runtimeNodes.begin() + selectedNodeIndex); // 選択したノードを消す

            if (selectedNodeIndex >= currentConfig.nodes.size())
            {
                selectedNodeIndex = static_cast<int>(currentConfig.nodes.size()) - 1;
            }
            SyncRuntimeNodes();
        }
    }
    ImGui::Separator();

    // ノード一覧
    for (int i = 0; i < currentConfig.nodes.size(); i++)
    {
        auto& node = currentConfig.nodes[i];
        bool isSelected = (selectedNodeIndex == i);

        ImGui::PushID(i); // ID被り防止

        // 表示 / 非表示 切り替えボタン（目アイコン風）
        const char* eyeIcon = node.visible ? u8" [ON] " : u8"[OFF]";
        if (node.visible)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f)); // 緑色
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.2f, 0.2f, 1.0f)); // 赤色
        }

        if (ImGui::Button(eyeIcon))
        {
            node.visible = !node.visible;
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // ノードの選択ラベル
        std::string label = u8"[スプライト] ";
        if (node.type == RenderType::Mesh) label = u8"[モデル] ";
        else if (node.type == RenderType::Trail) label = u8"[トレイル] ";
        label += node.name;

        if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            selectedNodeIndex = i;
        }

        ImGui::PopID();
    }
    ImGui::Separator();
}

// エフェクトを出す
void EffectEditor::EmitParticles(float elapsedTime)
{
    // 全ノードを順番にチェックして発生させる
    for (size_t i = 0; i < currentConfig.nodes.size(); ++i)
    {
        auto& cNode = currentConfig.nodes[i];
        auto& rNode = runtimeNodes[i];

        // パーティクルノード以外、または非表示ノードはスキップ
        if (cNode.type != RenderType::Particle || !cNode.visible) continue;
        if (effectTimer < cNode.delayTime) continue;

        float activeTime = effectTimer - cNode.delayTime;
        if (cNode.duration > 0.0f && activeTime >= cNode.duration) continue;

        // ノード個別のタイマーを進める
        rNode.spawnTimer += elapsedTime;

        if (rNode.spawnTimer >= cNode.spawnInterval)
        {
            rNode.spawnTimer -= cNode.spawnInterval;

            // そのノードのテクスチャをGPUにセット（マルチテクスチャ対応！）
            if (rNode.particleManager && !cNode.texturePath.empty())
            {
                rNode.particleManager->SetTexture(cNode.texturePath, { (UINT)cNode.splitCount[0], (UINT)cNode.splitCount[1] });
            }

            // 生成数分 for 文で回す
            for (int emitIdx = 0; emitIdx < cNode.emitCount; emitIdx++)
            {
                compute_particle_system::emit_particle_data data;
                data.parameter = { 0.0f, 0.0f, 0.0f, 0.0f };

                // パーティクルのデータを送る
                data.parameter.x = static_cast<float>(cNode.spriteType); // コマ番号
                data.parameter.y = cNode.lifeTime; // 寿命
                data.parameter.z = cNode.uvScrollSpeed.x; // x軸のUVスクロール
                data.parameter.w = cNode.uvScrollSpeed.y; // y軸のUVスクロール

                // フェードのスピードを設定する
                int packedSpeed =
                    (cNode.fadeInStartSpeed)
                    | (cNode.fadeInEndSpeed << 4)
                    | (cNode.fadeOutStartSpeed << 8)
                    | (cNode.fadeOutEndSpeed << 12);

                data.fade_param.x = cNode.IsFadeIn ? cNode.fadeInTime : 0.0f;
                data.fade_param.y = cNode.IsFadeOut ? cNode.fadeOutTime : 0.0f;
                data.fade_param.z = cNode.lifeTime;
                data.fade_param.w = static_cast<float>(packedSpeed);


                // 位置
                data.position = {
                   cNode.position.x + (cNode.positionRange.x * Mathf::RandFloat()),
                   cNode.position.y + (cNode.positionRange.y * Mathf::RandFloat()),
                   cNode.position.z + (cNode.positionRange.z * Mathf::RandFloat()),
                   1.0f
                };
                // 初速度
                data.posVelocity = {
                    cNode.positionVelocity.x + (cNode.positionVelocityRange.x * Mathf::RandFloat()),
                    cNode.positionVelocity.y + (cNode.positionVelocityRange.y * Mathf::RandFloat()),
                    cNode.positionVelocity.z + (cNode.positionVelocityRange.z * Mathf::RandFloat()),
                    0.0f
                };
                // 加速度
                data.posAcceleration = {
                    cNode.positionAcceleration.x + (cNode.positionAccelerationRange.x * Mathf::RandFloat()),
                    cNode.positionAcceleration.y + (cNode.positionAccelerationRange.y * Mathf::RandFloat()),
                    cNode.positionAcceleration.z + (cNode.positionAccelerationRange.z * Mathf::RandFloat()),
                    0.0f
                };

                // 回転
                data.rotation = {
                    DirectX::XMConvertToRadians(cNode.rotation.x + (cNode.rotationRange.x * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(cNode.rotation.y + (cNode.rotationRange.y * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(cNode.rotation.z + (cNode.rotationRange.z * Mathf::RandFloat())),
                    cNode.isBillboard ? 1.0f : 0.0f
                };
                data.rotVelocity = {
                    DirectX::XMConvertToRadians(cNode.rotationVelocity.x + (cNode.rotationVelocityRange.x * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(cNode.rotationVelocity.y + (cNode.rotationVelocityRange.y * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(cNode.rotationVelocity.z + (cNode.rotationVelocityRange.z * Mathf::RandFloat())),
                    0.0f
                };
                data.rotAcceleration = {
                    DirectX::XMConvertToRadians(cNode.rotationAcceleration.x + (cNode.rotationAccelerationRange.x * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(cNode.rotationAcceleration.y + (cNode.rotationAccelerationRange.y * Mathf::RandFloat())),
                    DirectX::XMConvertToRadians(cNode.rotationAcceleration.z + (cNode.rotationAccelerationRange.z * Mathf::RandFloat())),
                    0.0f
                };

                int scaleEasingSpeed =
                    (cNode.scaleUniformStartSpeed)
                    | (cNode.scaleUniformEndSpeed << 4);

                // 拡大
                switch (cNode.scaleMode)
                {
                case ScaleMode::Fixed: // 固定
                    data.scale = {
                        std::max(0.0f, cNode.scale.x + (cNode.scaleRange.x * Mathf::RandFloat())),
                        std::max(0.0f, cNode.scale.y + (cNode.scaleRange.y * Mathf::RandFloat())),
                        1.0f, 0.0f
                    };
                    data.scaleVelocity = {
                        cNode.scaleVelocity.x + (cNode.scaleVelocityRange.x * Mathf::RandFloat()),
                        cNode.scaleVelocity.y + (cNode.scaleVelocityRange.y * Mathf::RandFloat()),
                        cNode.scaleVelocity.z + (cNode.scaleVelocityRange.z * Mathf::RandFloat()),
                        0.0f
                    };
                    data.scaleAcceleration = {
                        cNode.scaleAcceleration.x + (cNode.scaleAccelerationRange.x * Mathf::RandFloat()),
                        cNode.scaleAcceleration.y + (cNode.scaleAccelerationRange.y * Mathf::RandFloat()),
                        cNode.scaleAcceleration.z + (cNode.scaleAccelerationRange.z * Mathf::RandFloat()),
                        0.0f
                    };

                    data.scale_param.x = -1.0f; // イージング無しのフラグ

                    break;
                case ScaleMode::UniformEasing: // 単一イージング

                    // 中心 ± 振れ幅の範囲でランダムに実際の値を決める
                    float actualStart = cNode.scaleUniformStart
                        + Mathf::RandomRange(-cNode.scaleUniformStartRandom, cNode.scaleUniformStartRandom);
                    float actualEnd = cNode.scaleUniformEnd
                        + Mathf::RandomRange(-cNode.scaleUniformEndRandom, cNode.scaleUniformEndRandom);

                    data.scaleUniformStart = actualStart;
                    data.scaleUniformEnd = actualEnd;
                    data.scale_param.x = scaleEasingSpeed;
                    data.scale_param.y = cNode.lifeTime;
                    break;
                }

                int colorEasingSpeed =
                    (cNode.colorStartSpeed)
                    | (cNode.colorEndSpeed << 4);

                // 色
                switch (cNode.colorMode)
                {
                case ColorMode::Fixed: // 固定
                    data.color = cNode.color;
                    data.start_color = cNode.color;
                    data.end_color = cNode.color;
                    data.color_param.x = -1.0f; // イージング無しのフラグ
                    data.color_param.y = cNode.lifeTime;

                    break;

                case ColorMode::Random: // ランダム
                    // 色をランダムで取得してcolorに入れる
                    data.color.x = Mathf::RandomRange(cNode.colorMin.x, cNode.colorMax.x);
                    data.color.y = Mathf::RandomRange(cNode.colorMin.y, cNode.colorMax.y);
                    data.color.z = Mathf::RandomRange(cNode.colorMin.z, cNode.colorMax.z);
                    data.color.w = Mathf::RandomRange(cNode.colorMin.w, cNode.colorMax.w);

                    data.start_color = data.color;
                    data.end_color = data.color;
                    data.color_param.x = -1.0f;
                    data.color_param.y = cNode.lifeTime;


                    break;

                case ColorMode::Easing: // イージング
                {
                    // 始点の色を指定した範囲でランダムに取得
                    data.start_color.x = Mathf::RandomRange(cNode.colorStartMin.x, cNode.colorStartMax.x);
                    data.start_color.y = Mathf::RandomRange(cNode.colorStartMin.y, cNode.colorStartMax.y);
                    data.start_color.z = Mathf::RandomRange(cNode.colorStartMin.z, cNode.colorStartMax.z);
                    data.start_color.w = Mathf::RandomRange(cNode.colorStartMin.w, cNode.colorStartMax.w);

                    // 終点の色を指定した範囲でランダムに取得
                    data.end_color.x = Mathf::RandomRange(cNode.colorEndMin.x, cNode.colorEndMax.x);
                    data.end_color.y = Mathf::RandomRange(cNode.colorEndMin.y, cNode.colorEndMax.y);
                    data.end_color.z = Mathf::RandomRange(cNode.colorEndMin.z, cNode.colorEndMax.z);
                    data.end_color.w = Mathf::RandomRange(cNode.colorEndMin.w, cNode.colorEndMax.w);

                    data.color_param.x = colorEasingSpeed;
                    data.color_param.y = cNode.lifeTime;

                    break;
                }
                }


                rNode.particleManager->Emit(data);
            }

        }
    }
}

// ノード数に合わせて runtimeNodes のサイズやリソースを更新する関数
void EffectEditor::SyncRuntimeNodes()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();
    runtimeNodes.resize(currentConfig.nodes.size());

    for (size_t i = 0; i < currentConfig.nodes.size(); ++i)
    {
        auto& node = currentConfig.nodes[i];
        auto& rNode = runtimeNodes[i];

        if (node.type == RenderType::Particle && !rNode.particleManager)
        {
            rNode.particleManager = std::make_unique<Compute_Particle>(device);

            if (!node.texturePath.empty())
            {
                D3D11_TEXTURE2D_DESC texture2d_desc;
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = GpuResourceUtils::LoadTexture(device, node.texturePath.c_str(), srv.GetAddressOf(), &texture2d_desc);

                rNode.shaderResourceView = srv;
            }
        }
        // メッシュの場合、モデルが未ロード or パスが変わっていたらロード
        if (node.type == RenderType::Mesh && !node.meshPath.empty())
        {
            if (!rNode.meshModel)
            {
                rNode.meshModel = std::make_shared<Model>(device, node.meshPath.c_str());
            }

            if (!node.texturePath.empty())
            {
                D3D11_TEXTURE2D_DESC texture2d_desc;
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = GpuResourceUtils::LoadTexture(device, node.texturePath.c_str(), srv.GetAddressOf(), &texture2d_desc);

                rNode.shaderResourceView = srv;
            }

            if (!node.noisePath.empty())
            {
                D3D11_TEXTURE2D_DESC texture2d_desc;
                Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
                HRESULT hr = GpuResourceUtils::LoadTexture(device, node.noisePath.c_str(), srv.GetAddressOf(), &texture2d_desc);

                rNode.noiseShaderResourceView = srv;
            }
        }
        // トレイルの場合、未初期化なら生成
        else if (node.type == RenderType::Trail)
        {
            if (!rNode.trail)
            {
                rNode.trail = std::make_shared<Trail>();
                rNode.trail->Initialize();
            }
        }
    }
}