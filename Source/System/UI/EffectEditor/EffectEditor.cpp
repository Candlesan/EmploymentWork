#include "EffectEditor.h"
#include "System/UI/Dialog.h"
#include "System/Graphic/Graphics.h"
#include "System/Renderer/Shader/meshVFX/meshVFX.h"
#include <imgui.h>
#include <ctime>
#include <cstdlib>

// 初期化
void EffectEditor::Initialize(ID3D11Device* device)
{
	 // パーティクルマネージャーの初期化
	particleManager = std::make_unique<Compute_Particle>(device);

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

    DrawStateRangeTimeline();
    ImGui::Separator();

    DrawNodeList();
    ImGui::Separator();


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
    isStop = false;
    globalAlpha = 1.0f;

    // 各種タイマーをリセット！
    effectTimer = 0.0f;
    currentMeshTime = 0.0f;

    for (auto& node : currentConfig.nodes)
    {
        if (node.type == RenderType::Particle)
        {
            node.spawnTimer = 0.0f;
        }
    }

    // 最初の1回目の発生！
    //EmitParticles(0.0f);
}

// 更新処理
void EffectEditor::Update(ID3D11DeviceContext* dc, float elapsedTime)
{
    // パーティクルの計算を進める
    if (particleManager)
    {
        particleManager->Update(dc, elapsedTime);
    }

    // 再生中じゃなければここで終了
    if (!isPlaying) return;

    // エフェクト全体の時間を進める
    effectTimer += elapsedTime;
    SyncRuntimeNodes();

    // 各rangeを取得
    const StateRange* startRange = nullptr;
    const StateRange* loopRange = nullptr;
    const StateRange* endRange = nullptr;
    for (const auto& r : currentConfig.ranges)
    {
        if (r.name == "Start") startRange = &r;
        if (r.name == "Loop") loopRange = &r;
        if (r.name == "End")   endRange = &r;
    }

    // スタート範囲内だったらフェードインさせる
    if (startRange && effectTimer <= startRange->end)
    {
        float duration = startRange->end - startRange->start;
        if (duration > 0.0001f)
        {
            float rate = (effectTimer - startRange->start) / duration;
            globalAlpha = std::clamp(rate, 0.0f, 1.0f);
        }
    }
    else if (endRange && effectTimer >= endRange->start)    // エンド範囲内だったらフェードアウトさせる
    {
        float duration = endRange->end - endRange->start;
        if (duration > 0.0001f)
        {
            float rate = (effectTimer - endRange->start) / duration;
            globalAlpha = std::clamp(1.0f - rate, 0.0f, 1.0f);
        }
        else
        {
            globalAlpha = 0.0f;
        }
    }

    if (loopRange && !isStop)
    {
        // stopが呼ばれてなくて、ループの終端に達したらループ開始地点に巻き戻す
        if (effectTimer >= loopRange->end)
        {
            // 超えた時間分も合わせて巻き戻す
            float loopDuration = loopRange->end - loopRange->start; // ループの長さを求める
            if (loopDuration > 0.0001f)
            {
                effectTimer = loopRange->start + fmodf(effectTimer - loopRange->start, loopDuration);
            }
        }
    }

    //　ノードごとに走査する
    bool anyNodeActive = false; // まだ再生しているノードがあるか

    float maxDuration = 0.0f;

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

        // ノード個別の色×全ターのフェード率
        DirectX::XMFLOAT4 finalColor = node.color;
        finalColor.w *= globalAlpha;

        // パーティクルノードの処理
        if (node.type == RenderType::Particle)
        {
            // ノードの生存時間をチェック
            if (node.duration > maxDuration)
            {
                maxDuration = node.duration;
            }


        }
        // メッシュの処理
        else if (node.type == RenderType::Mesh)
        {
            if (effectTimer < node.delayTime) continue;

            if (rNode.meshModel)
            {
                float t = effectTimer - node.delayTime;

                // 拡大時間経過で大きく/小さくする
                // scale = 初期拡大 + (拡大速度 * t) + (0.5 * 拡大加速度 * t^2)　物理の計算らしい
                DirectX::XMFLOAT3 currentScale = {
                    node.scale.x + (node.scaleVelocity.x * t) + (0.5f * node.scaleAcceleration.x * t * t),
                    node.scale.y + (node.scaleVelocity.y * t) + (0.5f * node.scaleAcceleration.y * t * t),
                    node.scale.z + (node.scaleVelocity.z * t) + (0.5f * node.scaleAcceleration.z * t * t)
                };

                // マイナスにならないようにガード
                currentScale.x = (std::max)(0.0f, currentScale.x);
                currentScale.y = (std::max)(0.0f, currentScale.y);
                currentScale.z = (std::max)(0.0f, currentScale.z);

                DirectX::XMMATRIX S = DirectX::XMMatrixScaling(currentScale.x, currentScale.y, currentScale.z);

                // 回転
                DirectX::XMFLOAT3 currentRot = {
                            node.rotation.x + (node.rotationVelocity.x * t) + (0.5f * node.rotationAcceleration.x * t * t),
                            node.rotation.y + (node.rotationVelocity.y * t) + (0.5f * node.rotationAcceleration.y * t * t),
                            node.rotation.z + (node.rotationVelocity.z * t) + (0.5f * node.rotationAcceleration.z * t * t)
                };
                DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
                    DirectX::XMConvertToRadians(currentRot.x),
                    DirectX::XMConvertToRadians(currentRot.y),
                    DirectX::XMConvertToRadians(currentRot.z)
                );

                // 位置
                DirectX::XMFLOAT3 currentPos = {
                            node.position.x + (node.positionVelocity.x * t) + (0.5f * node.positionAcceleration.x * t * t),
                            node.position.y + (node.positionVelocity.y * t) + (0.5f * node.positionAcceleration.y * t * t),
                            node.position.z + (node.positionVelocity.z * t) + (0.5f * node.positionAcceleration.z * t * t)
                };

                DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
                    testSpawnPos.x + currentPos.x,
                    testSpawnPos.y + currentPos.y,
                    testSpawnPos.z + currentPos.z
                );

                // トランスフォーム行列を合成してモデルに適用
                DirectX::XMFLOAT4X4 worldTransform;
                DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);
                rNode.meshModel->UpdateTransform(worldTransform);
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

            if (node.lifeTime > maxDuration) maxDuration = node.lifeTime;
        }

        // 最大値を超えないようにする
        if (node.duration > maxDuration) maxDuration = node.duration;
        if (node.lifeTime > maxDuration) maxDuration = node.lifeTime;
    }

    // EmitParticle内で全ノードを回して生成する
    EmitParticles(elapsedTime);

    // EndRange が設定されている場合は End の終端をエフェクトの寿命とする
    if (endRange)
    {
        maxDuration = (std::max)(maxDuration, endRange->end);
    }

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
        }

        if (isLoop && !loopRange && !isStop)
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

            if (node.type == RenderType::Trail && rNode.trail)
            {
                rNode.trail->Render(rc);
            }
        }
    }

    // パーティクルの描画
    if (particleManager)
    {
        particleManager->Render(rc);
    }
}

// メッシュVFXの描画
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

        if (!node.visible || node.type != RenderType::Mesh) continue;

        if (rNode.meshModel && currentMeshTime <= node.lifeTime)
        {
            // このノード専用のパラメータをシェーダーにセット
            shader->SetCurrentTextures(rNode.shaderResourceView.Get(), rNode.noiseShaderResourceView.Get());
            shader->SetDistortionPower(node.distortionPower);

            DirectX::XMFLOAT2 noiseUvOffset = {
                node.noiseUvOffset.x * currentMeshTime,
                node.noiseUvOffset.y * currentMeshTime
            };
            shader->SetNoiseUvOffset(noiseUvOffset);
            shader->SetBlendMode(node.blendMode);

            DirectX::XMFLOAT4 meshColor = node.color;
            meshColor.w *= globalAlpha;
            shader->SetColor(meshColor); // このノード固有の色
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
    }
}


// UIの描画パーツ
void EffectEditor::DrawStateRangeTimeline()
{
    ImGui::Text(u8"ステートタイムライン");

    // 全体の時間を計算
    float totalTime = 0.0f;
    for (auto& node : currentConfig.nodes)
    {
        totalTime = node.duration;
    }

    for (const auto& r : currentConfig.ranges)
    {
        totalTime = (std::max)(totalTime, r.end);
    }
    for (const auto& node : currentConfig.nodes)
    {
        float nodeEnd = node.delayTime + (std::max)(node.duration, node.lifeTime);
        totalTime = (std::max)(totalTime, nodeEnd);
    }

    const float trackHeight = 24.0f; // シーケンサーの高さ
    const float labelWidth = 60.0f; // ラベルの幅
    const float handleWidth = 6.0f; // Rangeの取っ手の大きさ

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      cursor = ImGui::GetCursorScreenPos();
    float       width = ImGui::GetContentRegionAvail().x - labelWidth;
    if (width <= 0.0f)
    {
        width = 1.0f;
    }

    // 時刻をX座標に変換
    auto timeToX = [&](float t) -> float{
            return cursor.x + labelWidth + (t / totalTime) * width;
    };

    for (int i = 0; i < (int)currentConfig.ranges.size(); i++)
    {
        auto& r = currentConfig.ranges[i];
        bool  isSel = (selectedRangeIndex == i);

        ImGui::PushID(100 + i);

        ImVec2 rowPos = ImGui::GetCursorScreenPos();
        ImVec2 rTrackMin = { rowPos.x + labelWidth, rowPos.y };
        ImVec2 rTrackMax = { rTrackMin.x + width, rTrackMin.y + trackHeight };

        // ラベル
        ImGui::Text(r.name.c_str());
        ImGui::SameLine(labelWidth);

        // トラック背景
        dl->AddRectFilled(rTrackMin, rTrackMax, IM_COL32(40, 40, 40, 80), 4.0f);

        float sx = timeToX(r.start);
        float ex = timeToX(r.end);

        // 名前ごとに色を変える
        ImU32 col = IM_COL32(80, 150, 220, 180);
        if (r.name == "Start") col = IM_COL32(50, 180, 80, 180);
        else if (r.name == "Loop")  col = IM_COL32(200, 160, 40, 180);
        else if (r.name == "End")   col = IM_COL32(220, 60, 60, 180);

        // Range本体
        dl->AddRectFilled({ sx, rTrackMin.y + 2 }, { ex, rTrackMax.y - 2 }, col, 3.0f);
        if (isSel)
        {
            dl->AddRect({ sx, rTrackMin.y + 2 }, { ex, rTrackMax.y - 2 }, col, 3.0f);
        }

        // 左端ハンドル描画
        dl->AddRectFilled({ sx, rTrackMin.y + 2 }, { sx + handleWidth, rTrackMax.y - 2 }, IM_COL32(255, 255, 255, 200));
        // 右端ハンドル描画
        dl->AddRectFilled({ ex - handleWidth, rTrackMin.y + 2 }, { ex, rTrackMax.y - 2 }, IM_COL32(255, 255, 255, 200));

        // 中央ドラッグ（Range全体移動）
        float centerW = std::max(0.0f, (ex - sx) - handleWidth * 2);
        if (centerW > 0.0f)
        {
            ImGui::SetCursorScreenPos({ sx + handleWidth, rTrackMin.y });
            ImGui::InvisibleButton("##move", { centerW, trackHeight });
            if (ImGui::IsItemClicked()) { selectedRangeIndex = i; }
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
            {
                float dt = ImGui::GetIO().MouseDelta.x / width * totalTime;
                float len = r.end - r.start;
                r.start = std::clamp(r.start + dt, 0.0f, totalTime - len);
                r.end = r.start + len;
                selectedRangeIndex = i;
            }
        }

        // 左端ハンドル（start移動）
        ImGui::SetCursorScreenPos({ sx, rTrackMin.y });
        ImGui::InvisibleButton("##start", { handleWidth, trackHeight });
        if (ImGui::IsItemClicked()) { selectedRangeIndex = i; }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        {
            float dt = ImGui::GetIO().MouseDelta.x / width * totalTime;
            r.start = std::clamp(r.start + dt, 0.0f, std::max(0.0f, r.end - 0.01f));
            selectedRangeIndex = i;
        }

        // 右端ハンドル（end移動）
        ImGui::SetCursorScreenPos({ ex - handleWidth, rTrackMin.y });
        ImGui::InvisibleButton("##end", { handleWidth, trackHeight });
        if (ImGui::IsItemClicked()) { selectedRangeIndex = i; }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
        {
            float dt = ImGui::GetIO().MouseDelta.x / width * totalTime;
            r.end = std::clamp(r.end + dt, r.start + 0.01f, totalTime);
            selectedRangeIndex = i;
        }

        ImGui::SetCursorScreenPos({ rowPos.x, rowPos.y + trackHeight + 4 });
        ImGui::Dummy({ width + labelWidth, 0 });
        ImGui::PopID();
    }

    // 再生ヘッドの縦線（現在の effectTimer の位置）
    if (isPlaying)
    {
        float playX = timeToX(effectTimer);
        dl->AddLine({ playX, cursor.y }, { playX, cursor.y + (trackHeight + 4) * currentConfig.ranges.size() }, IM_COL32(255, 50, 50, 255), 2.0f);
    }

    ImGui::Spacing();
}
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
            ImGui::DragFloat3(u8"初期拡大##Scale", &node.scale.x, 0.01f);
            ImGui::DragFloat3(u8"拡大ばらつき##Scale", &node.scaleRange.x, 0.01f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"拡大速度##Scale", &node.scaleVelocity.x, 0.01f);
            ImGui::DragFloat3(u8"拡大速度ばらつき##Scale", &node.scaleVelocityRange.x, 0.01f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"拡大加速度##Scale", &node.scaleAcceleration.x, 0.01f);
            ImGui::DragFloat3(u8"拡大加速度ばらつき##Scale", &node.scaleAccelerationRange.x, 0.01f);

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

                    if (particleManager)
                    {
                        particleManager->SetTexture(
                            pathStr,
                            { (UINT)currentConfig.nodes[selectedNodeIndex].splitCount[0], (UINT)currentConfig.nodes[selectedNodeIndex].splitCount[1] }
                        );
                    }
                }
            }
            ImGui::SameLine();

            ImGui::Text(u8": % s", node.texturePath.c_str());

            if (!node.texturePath.empty())
            {
                if (ImGui::Button(u8"テクスチャ解除##TextuerClear"))
                {
                    node.texturePath = "";

                    if (particleManager)
                    {
                        particleManager->SetTexture("", { 1, 1 });
                    }
                }
            }

            ImGui::Separator();


            ImGui::Checkbox(u8"ビルボード##Particle", &node.isBillboard);
            ImGui::Separator();

            const char* blendItems[] = { u8"通常半透明 (Alpha)", u8"加算 (Additive)" };
            ImGui::Combo(u8"ブレンドモード##Mesh", &node.blendMode, blendItems, IM_ARRAYSIZE(blendItems));
            ImGui::Separator();


            if (ImGui::InputInt2(u8"画像分割数(X,Y)##Particle", node.splitCount))
            {
                // 分割数の数値をいじった瞬間にGPU側の設定も更新する
                if (particleManager && !node.texturePath.empty()) {
                    particleManager->SetTexture(node.texturePath, { (UINT)node.splitCount[0], (UINT)node.splitCount[1] });
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
            }
            ImGui::Separator();

            ImGui::ColorEdit4(u8"色##Color", &node.color.x);
            ImGui::Separator();

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
    if (ImGui::Button(u8"保存"))
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
    if (ImGui::Button(u8"読み込み"))
    {
        char filepath[MAX_PATH] = { 0 };
        HWND hwnd = Graphics::Instance().GetWindowHandle();
        if (Dialog::OpenFileName(filepath, MAX_PATH,
            "JSON File\0*.json\0\0",
            "ファイルを開く",
            hwnd) == DialogResult::OK)
        {
            currentConfig.Load(filepath);

            SyncRuntimeNodes();

            if (particleManager && !currentConfig.nodes[selectedNodeIndex].texturePath.empty())
            {
                particleManager->SetTexture(
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

    // テスト再生ボタン
    if (ImGui::Button(u8"再生"))
    {
        PlayTestEffect(testSpawnPos);
    }

    // プリセット Range追加ボタン
    if (ImGui::Button(u8"+ Start"))
    {
        if (!HasRange("Start")) currentConfig.ranges.push_back({ "Start", 0.0f, 0.5f }); 
    }
    ImGui::SameLine();
    if (ImGui::Button(u8"+ Loop"))
    {
        if (!HasRange("Loop")) currentConfig.ranges.push_back({ "Loop", 0.5f, 2.0f });
    }
    ImGui::SameLine();
    if (ImGui::Button(u8"+ End"))
    {
        if (!HasRange("End")) currentConfig.ranges.push_back({ "End", 2.0f, 2.5f });
    }

    // 選択されている Range があれば削除ボタンを有効化
    if (selectedRangeIndex >= 0 && selectedRangeIndex < (int)currentConfig.ranges.size())
    {
        ImGui::SameLine();
        if (ImGui::Button(u8"選択Range削除"))
        {
            currentConfig.ranges.erase(currentConfig.ranges.begin() + selectedRangeIndex);
                selectedRangeIndex = -1;
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
    }
    ImGui::SameLine();

    // ノードが1個しかない時は削除できないようにする
    if (currentConfig.nodes.size() > 1)
    {
        if (ImGui::Button(u8"- 選択ノード削除"))
        {
            currentConfig.nodes.erase(currentConfig.nodes.begin() + selectedNodeIndex);
            if (selectedNodeIndex >= currentConfig.nodes.size())
            {
                selectedNodeIndex = static_cast<int>(currentConfig.nodes.size()) - 1;
            }
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


// -1.0f～1.0fの間をランダムで返す関数
float EffectEditor::RandFloat()
{
    return (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 2.0f - 1.0f;
}

// エフェクトを出す
void EffectEditor::EmitParticles(float elapsedTime)
{
    // 全ノードを順番にチェックして発生させる
    for (auto& node : currentConfig.nodes)
    {
        // パーティクルノード以外、または非表示ノードはスキップ
        if (node.type != RenderType::Particle || !node.visible) continue;

        auto& pConfig = node;

        if (effectTimer < node.delayTime) continue;

        float activeTime = effectTimer - node.delayTime;
        if (node.duration > 0.0f && activeTime >= node.duration) continue;

        // ノード個別のタイマーを進める
        pConfig.spawnTimer += elapsedTime;

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
                   pConfig.position.x + (pConfig.positionRange.x * RandFloat()),
                   pConfig.position.y + (pConfig.positionRange.y * RandFloat()),
                   pConfig.position.z + (pConfig.positionRange.z * RandFloat()),
                   1.0f
                };
                // 初速度
                data.posVelocity = {
                    pConfig.positionVelocity.x + (pConfig.positionVelocityRange.x * RandFloat()),
                    pConfig.positionVelocity.y + (pConfig.positionVelocityRange.y * RandFloat()),
                    pConfig.positionVelocity.z + (pConfig.positionVelocityRange.z * RandFloat()),
                    0.0f
                };
                // 加速度
                data.posAcceleration = {
                    pConfig.positionAcceleration.x + (pConfig.positionAccelerationRange.x * RandFloat()),
                    pConfig.positionAcceleration.y + (pConfig.positionAccelerationRange.y * RandFloat()),
                    pConfig.positionAcceleration.z + (pConfig.positionAccelerationRange.z * RandFloat()),
                    0.0f
                };

                // 回転
                data.rotation = {
                    DirectX::XMConvertToRadians(pConfig.rotation.x + (pConfig.rotationRange.x * RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotation.y + (pConfig.rotationRange.y * RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotation.z + (pConfig.rotationRange.z * RandFloat())),
                    pConfig.isBillboard ? 1.0f : 0.0f
                };
                data.rotVelocity = {
                    DirectX::XMConvertToRadians(pConfig.rotationVelocity.x + (pConfig.rotationVelocityRange.x * RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationVelocity.y + (pConfig.rotationVelocityRange.y * RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationVelocity.z + (pConfig.rotationVelocityRange.z * RandFloat())),
                    0.0f
                };
                data.rotAcceleration = {
                    DirectX::XMConvertToRadians(pConfig.rotationAcceleration.x + (pConfig.rotationAccelerationRange.x * RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationAcceleration.y + (pConfig.rotationAccelerationRange.y * RandFloat())),
                    DirectX::XMConvertToRadians(pConfig.rotationAcceleration.z + (pConfig.rotationAccelerationRange.z * RandFloat())),
                    0.0f
                };

                // 拡大
                data.scale = {
                    std::max(0.0f, pConfig.scale.x + (pConfig.scaleRange.x * RandFloat())),
                    std::max(0.0f, pConfig.scale.y + (pConfig.scaleRange.y * RandFloat())),
                    1.0f, 0.0f
                };
                data.scaleVelocity = {
                    pConfig.scaleVelocity.x + (pConfig.scaleVelocityRange.x * RandFloat()),
                    pConfig.scaleVelocity.y + (pConfig.scaleVelocityRange.y * RandFloat()),
                    pConfig.scaleVelocity.z + (pConfig.scaleVelocityRange.z * RandFloat()),
                    0.0f
                };
                data.scaleAcceleration = {
                    pConfig.scaleAcceleration.x + (pConfig.scaleAccelerationRange.x * RandFloat()),
                    pConfig.scaleAcceleration.y + (pConfig.scaleAccelerationRange.y * RandFloat()),
                    pConfig.scaleAcceleration.z + (pConfig.scaleAccelerationRange.z * RandFloat()),
                    0.0f
                };

                data.color = pConfig.color;
                data.color.w *= globalAlpha; // 全体のフェード率を反映

                particleManager->Emit(data);
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

// そのラベルがあるかを調べる関数
bool EffectEditor::HasRange(std::string name)
{
    for (auto& r : currentConfig.ranges)
    {
        if (r.name == name)
        {
            return true;
        }
    }
    return false;
}
