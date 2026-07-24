#include "EffectEditor.h"
#include "System/UI/Dialog.h"
#include "System/Graphic/Graphics.h"
#include <imgui.h>
#include <ctime>
#include <cstdlib>

// 初期化
void EffectEditor::Initialize(ID3D11Device* device)
{
	 // パーティクルマネージャーの初期化
	particleManager = std::make_unique<Compute_Particle>(device);

    // トレイルの初期化
    trailRenderer = std::make_unique<Trail>();
    trailRenderer->Initialize();

    srand(static_cast<unsigned int>(time(nullptr)));
}

// GUI描画
void EffectEditor::DrawGUI()
{
	ImGui::Begin(u8"エフェクトエディター");

	// ツールバー
	DrawFileToolbar();
	ImGui::Separator();

    // ノード一覧
    DrawNodeList();
    ImGui::Separator();

    if (selectedNodeIndex >= currentConfig.nodes.size())
    {
        selectedNodeIndex = 0;
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

    if (selectedNode.type == EffectNodeType::Particle)
    {
        if (ImGui::CollapsingHeader(u8"パーティクル設定", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawParticleInspector(selectedNode.particle);
        }
    }
    else if (selectedNode.type == EffectNodeType::Mesh)
    {
        if (ImGui::CollapsingHeader(u8"メッシュVFX設定", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawMeshVfxInspector(selectedNode.meshVfx);
        }
    }
    // DrawGUI 内
    else if (selectedNode.type == EffectNodeType::Trail)
    {
        if (ImGui::CollapsingHeader(u8"トレイル設定", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawTrailInspector(selectedNode.trail);
        }
    }
    ImGui::End();
}

// テスト再生
void EffectEditor::PlayTestEffect(const DirectX::XMFLOAT3& spawnPosition)
{
    testSpawnPos = spawnPosition;
    isPlaying = true;

    // 各種タイマーをリセット！
    effectTimer = 0.0f;
    currentMeshTime = 0.0f;

    for (auto& node : currentConfig.nodes)
    {
        if (node.type == EffectNodeType::Particle)
        {
            node.particle.spawnTimer = node.particle.spawnInterval;
        }
    }

    // 最初の1回目の発生！
    EmitParticles(0.0f);
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

    //　ノードごとに走査する
    bool anyNodeActive = false; // まだ再生しているノードがあるか

    float maxDuration = 0.0f;

    for (auto& node : currentConfig.nodes)
    {
        if (!node.visible) continue;

        // パーティクルノードの処理
        if (node.type == EffectNodeType::Particle)
        {
            // ノードの生存時間をチェック
            if (node.particle.duration > maxDuration)
            {
                maxDuration = node.particle.duration;
            }
        }
        // メッシュの処理
        else if (node.type == EffectNodeType::Mesh)
        {
            if (effectTimer < node.meshVfx.delayTime) continue;

            currentMeshTime += elapsedTime;

            // Scale: XとYの設定しかないため、幅と奥行き(XとZ)を均等に拡縮する
            DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.meshVfx.scale.x, node.meshVfx.scale.y, node.meshVfx.scale.x);

            // Rotation: 回転スピード × 経過時間 で自転させる
            float rx = DirectX::XMConvertToRadians(node.meshVfx.rotationSpeed.x * currentMeshTime);
            float ry = DirectX::XMConvertToRadians(node.meshVfx.rotationSpeed.y * currentMeshTime);
            float rz = DirectX::XMConvertToRadians(node.meshVfx.rotationSpeed.z * currentMeshTime);
            DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(rx, ry, rz);

            // Translation: テスト発生位置
            DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(testSpawnPos.x, testSpawnPos.y, testSpawnPos.z);

            // トランスフォーム行列を合成してモデルに適用
            DirectX::XMFLOAT4X4 worldTransform;
            DirectX::XMStoreFloat4x4(&worldTransform, S * R * T);
            vfxMeshModel->UpdateTransform(worldTransform);
        }
        // トレイルの処理
        else if (node.type == EffectNodeType::Trail)
        {
            if (effectTimer < node.trail.delayTime) continue;

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
                root.y + node.trail.width, // 幅（太さ）分だけ上にする
                root.z
            };

            // Trailクラスに過去座標として記録させる
            if (trailRenderer)
            {
                trailRenderer->SetWidthScales(node.trail.widthStart, node.trail.widthMiddle, node.trail.widthEnd);
                trailRenderer->Update(root, tip);
            }

            if (node.trail.lifeTime > maxDuration) maxDuration = node.trail.lifeTime;
        }

        if (node.meshVfx.lifeTime > maxDuration)
        {
            maxDuration = node.meshVfx.lifeTime;
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
            if (node.type == EffectNodeType::Particle && node.particle.isLoop)
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
    // メッシュVFXの描画登録
    if (isPlaying && currentConfig.nodes[selectedNodeIndex].meshVfx.useMesh && vfxMeshModel)
    {
        // 寿命が来るまで描画
        if (currentMeshTime <= currentConfig.nodes[selectedNodeIndex].meshVfx.lifeTime)
        {
            modelRenderer->Draw(ShaderId::PBR, vfxMeshModel);
        }
    }

    // トレイルの描画
    if (isPlaying && trailRenderer && currentConfig.nodes[selectedNodeIndex].type == EffectNodeType::Trail)
    {
        trailRenderer->Render(rc);
    }

    // パーティクルの描画
    if (particleManager)
    {
        particleManager->Render(rc);
    }
}

// UIの描画パーツ
void EffectEditor::DrawParticleInspector(ParticleEffectConfig& particleConfig)
{
    if (ImGui::BeginTabBar(u8"ParticleTabs"))
    {
        //　共通タブ
        if (ImGui::BeginTabItem(u8"共通"))
        {
            ImGui::DragInt(u8"生成数##Particle", &particleConfig.emitCount, 1);
            ImGui::Separator();

            ImGui::Separator();

            ImGui::DragFloat(u8"寿命##Particle", &particleConfig.lifeTime, 0.05f, 0.1f, 10.0f);
            ImGui::Separator();

            ImGui::Checkbox(u8"ループ再生##Particle", &particleConfig.isLoop);
            ImGui::Separator();

            ImGui::DragFloat(u8"生成間隔(秒)##Particle", &particleConfig.spawnInterval, 0.01f, 0.01f, 5.0f, "%.2fs");
            ImGui::DragFloat(u8"存続時間(秒)##Particle", &particleConfig.duration, 0.1f, 0.0f, 60.0f, u8"%.1fs (0=無限)");
            ImGui::DragFloat(u8"発生遅延(秒)##Particle", &particleConfig.delayTime, 0.05f, 0.0f, 10.0f, "%.2fs");

            ImGui::EndTabItem();
        }

        // 位置タブ
        if (ImGui::BeginTabItem(u8"位置"))
        {
            ImGui::DragFloat3(u8"初期位置##Pos", &particleConfig.position.x, 0.1f);
            ImGui::DragFloat3(u8"位置ばらつき##Pos", &particleConfig.positionRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"初速度##Pos", &particleConfig.positionVelocity.x, 0.1f);
            ImGui::DragFloat3(u8"初速度ばらつき##Pos", &particleConfig.positionVelocityRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"加速度##Pos", &particleConfig.positionAcceleration.x, 0.1f);
            ImGui::DragFloat3(u8"加速度ばらつき##Pos", &particleConfig.positionAccelerationRange.x, 0.1f);

            ImGui::EndTabItem();
        }

        // 回転タブ
        if (ImGui::BeginTabItem(u8"回転"))
        {
            ImGui::DragFloat3(u8"初期回転##Rot", &particleConfig.rotation.x, 0.1f);
            ImGui::DragFloat3(u8"回転ばらつき##Rot", &particleConfig.rotationRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"角速度##Rot", &particleConfig.rotationVelocity.x, 0.1f);
            ImGui::DragFloat3(u8"角速度ばらつき##Rot", &particleConfig.rotationVelocityRange.x, 0.1f);
            ImGui::Separator();
            ImGui::DragFloat3(u8"回転加速度##Rot", &particleConfig.rotationAcceleration.x, 0.1f);
            ImGui::DragFloat3(u8"回転加速度ばらつき##Rot", &particleConfig.rotationAccelerationRange.x, 0.1f);

            ImGui::EndTabItem();
        }

        // 拡大（スケール）タブ
        if (ImGui::BeginTabItem(u8"拡大"))
        {
            ImGui::DragFloat2(u8"初期拡大##Scale", &particleConfig.scale.x, 0.01f);
            ImGui::DragFloat2(u8"拡大ばらつき##Scale", &particleConfig.scaleRange.x, 0.01f);
            ImGui::Separator();
            ImGui::DragFloat2(u8"拡大速度##Scale", &particleConfig.scaleVelocity.x, 0.01f);
            ImGui::DragFloat2(u8"拡大速度ばらつき##Scale", &particleConfig.scaleVelocityRange.x, 0.01f);
            ImGui::Separator();
            ImGui::DragFloat2(u8"拡大加速度##Scale", &particleConfig.scaleAcceleration.x, 0.01f);
            ImGui::DragFloat2(u8"拡大加速度ばらつき##Scale", &particleConfig.scaleAccelerationRange.x, 0.01f);

            ImGui::EndTabItem();
        }

        // 描画共通タブ
        if (ImGui::BeginTabItem(u8"描画共通"))
        {
            // テクスチャ選択
            if (ImGui::Button(u8"読込"))
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

                    currentConfig.nodes[selectedNodeIndex].particle.texturePath = pathStr;

                    if (particleManager)
                    {
                        particleManager->SetTexture(
                            pathStr,
                            { (UINT)currentConfig.nodes[selectedNodeIndex].particle.splitCount[0], (UINT)currentConfig.nodes[selectedNodeIndex].particle.splitCount[1] }
                        );
                    }
                }
            }
            ImGui::SameLine();

            ImGui::Text(u8": % s", particleConfig.texturePath.c_str());
            ImGui::Separator();

            ImGui::Checkbox(u8"ビルボード##Particle", &particleConfig.isBillboard); 
            ImGui::Separator();

            if (ImGui::InputInt2(u8"画像分割数(X,Y)##Particle", particleConfig.splitCount))
            {
                // 分割数の数値をいじった瞬間にGPU側の設定も更新する
                if (particleManager && !particleConfig.texturePath.empty()) {
                    particleManager->SetTexture(particleConfig.texturePath, { (UINT)particleConfig.splitCount[0], (UINT)particleConfig.splitCount[1] });
                }
            }

            ImGui::SliderInt(u8"コマ番号##Particle", &particleConfig.spriteType, 0, 15);
            ImGui::DragFloat2(u8"UVスクロール速度(X,Y)##Particle", &particleConfig.uvScrollSpeed.x, 0.01f, -10.0f, 10.0f, "%.2f");

            ImGui::EndTabItem();
        }

        // カラータブ
        if (ImGui::BeginTabItem(u8"カラー"))
        {
            ImGui::ColorEdit4(u8"色##Color", &particleConfig.color.x);

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar(); 
    }
}
void EffectEditor::DrawMeshVfxInspector(MeshVfxConfig& meshVfxConfig)
{
    ImGui::Checkbox(u8"Use Mesh VFX", &meshVfxConfig.useMesh);
    ImGui::Separator();

    if (meshVfxConfig.useMesh)
    {
        ImGui::Text(u8"Mesh: %s", meshVfxConfig.meshPath.c_str());
        ImGui::SameLine();
        if (ImGui::Button(u8"Select Mesh (.gltf/.glb)"))
        {
            char filePath[MAX_PATH] = { 0 };
            HWND hwnd = Graphics::Instance().GetWindowHandle();
            if (Dialog::OpenFileName(filePath, MAX_PATH,
                "GLTF/GLB File\0*.gltf;*.glb\0\0",
                "モデルを開く",
                hwnd) == DialogResult::OK)
            {
                std::string pathStr = filePath;

                // Dataという文字を探してそれ以降のパスを切り取る
                size_t pos = pathStr.find("Data");
                if (pos != std::string::npos)
                {
                    pathStr = pathStr.substr(pos);
                }

                meshVfxConfig.meshPath = pathStr;
                ID3D11Device* device = Graphics::Instance().GetDevice();
                vfxMeshModel = std::make_shared<Model>(device, pathStr.c_str());
            }
        }
        ImGui::Separator();

        ImGui::DragFloat(u8"寿命##Mesh", &meshVfxConfig.lifeTime, 0.05f, 0.1f, 10.0f);
        ImGui::ColorEdit4(u8"色##Mesh", &meshVfxConfig.color.x);
        ImGui::DragFloat2(u8"大きさ##Mesh", &meshVfxConfig.scale.x, 0.05f, 0.01f, 50.0f);
        ImGui::DragFloat3(u8"回転速度##Mesh", &meshVfxConfig.rotationSpeed.x, 1.0f);
        ImGui::DragFloat(u8"発生遅延(秒)##Mesh", &meshVfxConfig.delayTime, 0.05f, 0.0f, 10.0f, "%.2fs");
    }
}
void EffectEditor::DrawTrailInspector(TrailConfig& trailConfig)
{
    // テクスチャ選択
    if (ImGui::Button(u8"読み込み##Trail"))
    {
        char filePath[MAX_PATH] = { 0 };
        HWND hwnd = Graphics::Instance().GetWindowHandle();
        if (Dialog::OpenFileName(filePath, MAX_PATH,
            "PNG File\0*.png\0DDS File\0*.dds\0TGA File\0*.tga\0All Files\0*.*\0\0",
            "テクスチャ選択",
            hwnd) == DialogResult::OK)
        {
            std::string pathStr = filePath;

            // Dataという文字を探してそれ以降のパスを切り取る
            size_t pos = pathStr.find("Data");
            if (pos != std::string::npos)
            {
                pathStr = pathStr.substr(pos);
            }

            trailConfig.texturePath = pathStr;

            if (trailRenderer)
            {
                trailRenderer->SetTexture(pathStr);
            }
        }
    }
    ImGui::SameLine();
    ImGui::Text(u8": %s", trailConfig.texturePath.c_str());
    ImGui::Separator();

    ImGui::DragFloat(u8"寿命##Trail", &trailConfig.lifeTime, 0.05f, 0.1f, 10.0f);
    ImGui::DragFloat(u8"発生遅延(秒)##Trail", &trailConfig.delayTime, 0.05f, 0.0f, 10.0f, "%.2fs");
    ImGui::Separator();

    ImGui::DragFloat(u8"幅(太さ)##Trail", &trailConfig.width, 0.1f);
    ImGui::Separator();

    ImGui::Text(u8"【幅スケール (形状)】");
    ImGui::DragFloat(u8"先端(頭)##Trail", &trailConfig.widthStart, 0.01f, 0.0f, 5.0f, "%.2f");
    ImGui::DragFloat(u8"中央##Trail", &trailConfig.widthMiddle, 0.01f, 0.0f, 5.0f, "%.2f");
    ImGui::DragFloat(u8"末端(尻)##Trail", &trailConfig.widthEnd, 0.01f, 0.0f, 5.0f, "%.2f");
    ImGui::Separator();

    if (ImGui::ColorEdit4(u8"色##Trail", &trailConfig.color.x))
    {
        if (trailRenderer)
        {
            trailRenderer->SetColor(trailConfig.color);
        }
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
            currentConfig.Load(filepath);

            if (particleManager && !currentConfig.nodes[selectedNodeIndex].particle.texturePath.empty())
            {
                particleManager->SetTexture(
                    currentConfig.nodes[selectedNodeIndex].particle.texturePath,
                    { (UINT)currentConfig.nodes[selectedNodeIndex].particle.splitCount[0], (UINT)currentConfig.nodes[selectedNodeIndex].particle.splitCount[1] }
                );
            }

            // メッシュパスが有効ならモデルもリロード
            if (currentConfig.nodes[selectedNodeIndex].meshVfx.useMesh && !currentConfig.nodes[selectedNodeIndex].meshVfx.meshPath.empty())
            {
                ID3D11Device* device = Graphics::Instance().GetDevice();
                vfxMeshModel = std::make_shared<Model>(device, currentConfig.nodes[selectedNodeIndex].meshVfx.meshPath.c_str());
            }
        }
    }

	// テスト再生ボタン
	if (ImGui::Button(u8"再生"))
	{
		PlayTestEffect(testSpawnPos);
	}
}
void EffectEditor::DrawNodeList()
{
    ImGui::Text(u8"【ノード一覧】");

    if (ImGui::Button(u8"+ パーティクル追加"))
    {
        EffectNode newNode;
        newNode.name = "Particle Node" + std::to_string((currentConfig.nodes.size()) + 1);
        newNode.type = EffectNodeType::Particle;
        currentConfig.nodes.push_back(newNode);
        selectedNodeIndex = static_cast<int>(currentConfig.nodes.size() - 1); // 追加したものを選択する

    }
    ImGui::SameLine();
    if (ImGui::Button(u8"+ メッシュVFX追加"))
    {
        EffectNode newNode;
        newNode.name = "Mesh Node" + std::to_string((currentConfig.nodes.size()) + 1);
        newNode.type = EffectNodeType::Mesh;
        currentConfig.nodes.push_back(newNode);
        selectedNodeIndex = static_cast<int>(currentConfig.nodes.size() - 1); // 追加したものを選択する
    }
    ImGui::SameLine();
    if (ImGui::Button(u8"+ トレイル追加"))
    {
        EffectNode newNode;
        newNode.name = "Trail Node" + std::to_string((currentConfig.nodes.size()) + 1);
        newNode.type = EffectNodeType::Trail;
        currentConfig.nodes.push_back(newNode);
        selectedNodeIndex = static_cast<int>(currentConfig.nodes.size() - 1); // 追加したものを選択する
    }

    ImGui::Separator();

    // ノードのリストを表示
    if (ImGui::ListBoxHeader("##NodeListBox", ImVec2(-FLT_MIN, 120)))
    {
        for (int i = 0; i < currentConfig.nodes.size(); i++)
        {
            auto& node = currentConfig.nodes[i];

            bool isSelected = (selectedNodeIndex == i);

            // 種別アイコンの突きのラベル
            std::string label = u8"[粒] ";
            if (node.type == EffectNodeType::Mesh) label = u8"[網] ";
            else if (node.type == EffectNodeType::Trail) label = u8"[帯] ";
            label += node.name;

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                selectedNodeIndex = i;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::ListBoxFooter();
    }

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
}

// -1.0f～1.0fの間をランダムで返す関数
float EffectEditor::RandFloat()
{
    return (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 2.0f - 1.0f;
}

void EffectEditor::EmitParticles(float elapsedTime)
{
    // 全ノードを順番にチェックして発生させる
    for (auto& node : currentConfig.nodes)
    {
        // パーティクルノード以外、または非表示ノードはスキップ
        if (node.type != EffectNodeType::Particle || !node.visible) continue;

        auto& pConfig = node.particle;

        if (effectTimer < node.particle.delayTime) continue;

        float activeTime = effectTimer - node.particle.delayTime;
        if (node.particle.duration > 0.0f && activeTime >= node.particle.duration) continue;

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

                particleManager->Emit(data);
            }
        }
    }
}