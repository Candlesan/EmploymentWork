#include "EffectEditor.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffect.h"
#include "System/UI/Dialog/Dialog.h"
#include "System/Graphic/Graphics.h"
#include "System/Shader/meshVFX/meshVFX.h"
#include "System/Core/Mathf.h"

#include <imgui.h>

// CustomEffect の定義が見える場所でコンストラクタ・デストラクタを定義する（unique_ptr のため）
EffectEditor::EffectEditor() = default;
EffectEditor::~EffectEditor() = default;

// 初期化
void EffectEditor::Initialize(ID3D11Device* device)
{
    RebuildPreview();
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
    ImGui::SameLine();

    // 停止ボタン
    if (ImGui::Button(u8"停止"))
    {
        if (preview) preview->Stop();
    }
    ImGui::SameLine();

    // トレイルの確認用に、テスト位置を中心に円を描いて動かす
    ImGui::Checkbox(u8"円を描いて動かす", &isTestMove);

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
    testMoveTimer = 0.0f;

    // 前の再生で残っている粒子を消してから作り直す
    if (preview) preview->Stop();
    RebuildPreview();

    // 本番と同じ CustomEffect で再生する（角度0・等倍）
    preview->Play(testSpawnPos, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
}

// 更新処理
void EffectEditor::Update(ID3D11DeviceContext* dc, float elapsedTime)
{
    if (!preview) return;

    // GUIで変えた値をプレビューに反映する
    // モデル・テクスチャ・描画方法・ノード数が変わっていたら作り直す
    if (!preview->ApplyConfig(currentConfig))
    {
        bool wasPlaying = !preview->IsFinished();

        preview->Stop();
        RebuildPreview();

        // 再生中だったら、作り直したエフェクトで再生し直す
        if (wasPlaying)
        {
            preview->Play(testSpawnPos, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
        }
    }

    // テスト位置を中心に円を描くように動かす
    DirectX::XMFLOAT3 position = testSpawnPos;
    if (isTestMove)
    {
        testMoveTimer += elapsedTime;

        float speed = 5.0f;  // 速度
        float radius = 1.5f; // 円の半径
        float currentAngle = testMoveTimer * speed;

        position.x += cosf(currentAngle) * radius;
        position.z += sinf(currentAngle) * radius;
    }
    preview->SetPosition(position);

    preview->Update(elapsedTime);
}

// 描画処理
void EffectEditor::Render(const RenderContext& rc, ModelRenderer* modelRenderer)
{
    if (!preview) return;

    // CustomEffect::Render は const でない RenderContext を受け取るのでコピーして渡す
    RenderContext previewRc = rc;
    preview->Render(previewRc);
}

// メッシュVFX描画
void EffectEditor::RenderMeshVFX(ModelRenderer* modelRenderer)
{
    if (!preview) return;

    // シェーダーの開始処理（パイプライン設定）
    auto* shader = dynamic_cast<meshVFX*>(modelRenderer->GetShader(ShaderId::MeshVFX));
    if (!shader) return;

    RenderContext rc;
    rc.deviceContext = Graphics::Instance().GetDeviceContext();
    rc.renderState = Graphics::Instance().GetRenderState();
    rc.camera = &CameraManager::Instance().GetMainCamera(); // カメラ参照

    shader->Begin(rc);
    preview->RenderMeshVFX(shader, rc);
    shader->End(rc);
}

// 今の設定でプレビュー用のエフェクトを作り直す
void EffectEditor::RebuildPreview()
{
    preview = std::make_unique<CustomEffect>();
    preview->Initialize(Graphics::Instance().GetDevice(), currentConfig);
}

// ノード1つと、その子をツリーで描く
void EffectEditor::DrawNodeTreeItem(int index, int depth)
{
    // 壊れたデータでループしていても止まるように、深さに上限をつける
    if (depth > static_cast<int>(currentConfig.nodes.size())) return;

    auto& node = currentConfig.nodes[index];

    ImGui::PushID(node.id); // ID被り防止

    // 目玉（表示/非表示の切り替え）
    if (ImGui::Button(node.visible ? u8"o" : u8"-", ImVec2(28, 0)))
    {
        node.visible = !node.visible;
    }
    ImGui::SameLine();

    // 子がいるか調べる
    bool hasChild = false;
    for (const auto& other : currentConfig.nodes)
    {
        if (other.parentId == node.id) { hasChild = true; break; }
    }

    // フラグ
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_DefaultOpen;
    if (!hasChild)                   flags |= ImGuiTreeNodeFlags_Leaf;     // 子がいなければ矢印なし
    if (selectedNodeIndex == index)  flags |= ImGuiTreeNodeFlags_Selected; // 選択中ならハイライト

    // 表示名（種類 + 名前）
    std::string label = u8"[スプライト] ";
    if (node.type == RenderType::Mesh)  label = u8"[モデル] ";
    else if (node.type == RenderType::Trail) label = u8"[軌跡] ";
    else if (node.type == RenderType::Ring)  label = u8"[リング] ";
    else if (node.type == RenderType::None)  label = u8"[無し] ";
    label += node.name;

    bool open = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());

    // クリックで選択（開閉に関係なく TreeNodeEx の直後で見る）
    if (ImGui::IsItemClicked()) selectedNodeIndex = index;

    // ツリーを開いていたら子を描く
    if (open)
    {
        for (int i = 0; i < static_cast<int>(currentConfig.nodes.size()); ++i)
        {
            if (currentConfig.nodes[i].parentId == node.id)
            {
                DrawNodeTreeItem(i, depth + 1);
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

// UIの描画パーツ
void EffectEditor::DrawNodeInspector(NodeConfig& node)
{
    ImGui::Checkbox(u8"このノードを表示する##VisibleCheck", &node.visible);
    ImGui::Separator();

    // 親ノードの選択
    std::string currentParentName = u8"なし（エフェクト直下）";
    int parentIndex = currentConfig.FindNodeIndex(node.parentId);
    if (parentIndex != -1) currentParentName = currentConfig.nodes[parentIndex].name;

    if (ImGui::BeginCombo(u8"親ノード##Parent", currentParentName.c_str()))
    {
        if (ImGui::Selectable(u8"なし（エフェクト直下）", node.parentId == -1))
        {
            node.parentId = -1;
        }

        for (const auto& candidate : currentConfig.nodes)
        {
            // 自分自身と、自分の子孫は親にできない（ループ防止）
            if (candidate.id == node.id) continue;
            if (currentConfig.IsDescendant(candidate.id, node.id)) continue;

            ImGui::PushID(candidate.id);
            if (ImGui::Selectable(candidate.name.c_str(), node.parentId == candidate.id))
            {
                node.parentId = candidate.id;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
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
            ImGui::Separator();

            if (node.type == RenderType::Particle)
            {
                const char* followItems[] = { u8"生成時のみ", u8"常に追従" };
                ImGui::Combo(u8"親への追従", &node.particleFollowMode, followItems, IM_ARRAYSIZE(followItems));
            }

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

                    // パスが変わるとプレビューが自動で作り直される（Update の ApplyConfig）
                    currentConfig.nodes[selectedNodeIndex].texturePath = pathStr;
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
                }
            }

            ImGui::Separator();

            const char* billboardItems[] = { u8"なし", u8"ビルボード", u8"速度方向に伸ばす" };
            ImGui::Combo(u8"ビルボード##Particle", &node.billboardType, billboardItems, IM_ARRAYSIZE(billboardItems));
            if (node.billboardType == 2)
            {
                ImGui::DragFloat(u8"伸ばす量##Stretch", &node.stretchScale, 0.01f, 0.0f, 10.0f);
            }
            ImGui::Separator();

            const char* blendItems[] = { u8"通常半透明 (Alpha)", u8"加算 (Additive)" };
            ImGui::Combo(u8"ブレンドモード##Mesh", &node.blendMode, blendItems, IM_ARRAYSIZE(blendItems));
            ImGui::Separator();

            const char* speedItem[] = { u8"低速3", u8"低速2",  u8"低速1",  u8"等速",  u8"高速1",  u8"高速2",  u8"高速3" };
            ImGui::Checkbox(u8"フェードイン", &node.IsFadeIn);
            if (node.IsFadeIn)
            {
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

            const char* dissolveItems[] = { u8"無し", u8"ディゾルブ",  u8"トゥーン調ディゾルブ" };
            ImGui::Combo(u8"ディゾルブ", &node.dissolveMode, dissolveItems, IM_ARRAYSIZE(dissolveItems));
            if (node.dissolveMode == 1 || node.dissolveMode == 2)
            {
                ImGui::DragFloat(u8"輪郭のぼかし幅", &node.dissolveSoftness, 0.01f, 0.0f, 1.0f);
                ImGui::Separator();
            }

            // 分割数は粒子を出すたびに CustomEffect 側で GPU に設定される
            ImGui::InputInt2(u8"画像分割数(X,Y)##Particle", node.splitCount);

            ImGui::SliderInt(u8"コマ番号##Particle", &node.spriteType, 0, 15);
            ImGui::DragFloat2(u8"UVスクロール速度(X,Y)##Particle", &node.uvScrollSpeed.x, 0.01f, -10.0f, 10.0f, "%.2f");

            ImGui::EndTabItem();
        }

        // 描画タブ
        if (ImGui::BeginTabItem(u8"描画"))
        {
            // 画面に出す順番と、それぞれの RenderType（番号は JSON と互換のため変えない）
            const RenderType renderTypeOrder[] = {
                RenderType::None,
                RenderType::Particle,
                RenderType::Mesh,
                RenderType::Trail,
                RenderType::Ring,
            };
            const char* renderTypeItems[] = {
                u8"無し",
                u8"スプライト",
                u8"モデル",
                u8"軌跡",
                u8"リング",
            };

            int currentType = 0;
            for (int i = 0; i < IM_ARRAYSIZE(renderTypeOrder); ++i)
            {
                if (renderTypeOrder[i] == node.type) { currentType = i; break; }
            }

            if (ImGui::Combo(u8"描画方法##RenderTypeCombo", &currentType, renderTypeItems, IM_ARRAYSIZE(renderTypeItems)))
            {
                node.type = renderTypeOrder[currentType];
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
                ImGui::DragFloat(u8"長さ(秒)", &node.trailLength, 0.01f);
                ImGui::DragFloat(u8"幅(太さ)", &node.width, 0.1f);
                ImGui::Separator();

                ImGui::Text(u8"【幅スケール (形状)】");
                ImGui::DragFloat(u8"先端(頭)", &node.widthStart, 0.01f, 0.0f, 5.0f);
                ImGui::DragFloat(u8"中央", &node.widthMiddle, 0.01f, 0.0f, 5.0f);
                ImGui::DragFloat(u8"末端(尻)", &node.widthEnd, 0.01f, 0.0f, 5.0f);
            }
            else if (node.type == RenderType::Ring)
            {
                ImGui::SliderInt(u8"頂点数##Ring", &node.ringDivision, 3, 128);
                ImGui::Separator();

                ImGui::DragFloat(u8"中心比率", &node.ringCenterRatio, 0.01f);
                ImGui::Separator();

                ImGui::Text(u8"外輪");
                ImGui::Text(u8"位置");
                ImGui::SameLine();
                ImGui::DragFloat2(u8"##Outer", &node.ringOuter.x, 0.01f);
                ImGui::Separator();

                ImGui::Text(u8"内輪");
                ImGui::Text(u8"位置");
                ImGui::SameLine();
                ImGui::DragFloat2(u8"##Inner", &node.ringInner.x, 0.01f);
                ImGui::Separator();

                ImGui::DragFloat(u8"開始角度##Ring", &node.ringStartAngle, 1.0f, -360.0f, 360.0f);
                ImGui::DragFloat(u8"終了角度##Ring", &node.ringEndAngle, 1.0f, -360.0f, 360.0f);

                ImGui::ColorEdit4(u8"外輪色", &node.ringOuterColor.x);
                ImGui::ColorEdit4(u8"中心色", &node.ringCenterColor.x);
                ImGui::ColorEdit4(u8"内輪色", &node.ringInnerColor.x);
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(u8"生成方法"))
        {
            const char* emitShapeItems[] = { u8"箱", u8"放射" };
            ImGui::Combo(u8"生成方法", &node.emitShape, emitShapeItems, IM_ARRAYSIZE(emitShapeItems));
            ImGui::Separator();

            if (node.emitShape == 0)
            {

            }
            else if (node.emitShape == 1)
            {
                ImGui::DragFloat(u8"最小速度##emitShape", &node.burstSpeedMin, 0.01f);
                ImGui::DragFloat(u8"最大速度##emitShape", &node.burstSpeedMax, 0.01f);
                ImGui::DragFloat(u8"空気抵抗##emitShape", &node.drag, 0.01f, 0.0f);
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
            currentConfig.Load(filepath);

            // 読み込んだノード数を超えていたら安全な範囲に戻す
            if (selectedNodeIndex >= static_cast<int>(currentConfig.nodes.size()))
            {
                selectedNodeIndex = 0;
            }

            // 前のファイルのプレビューを止めて、読み込んだ設定で作り直す
            if (preview) preview->Stop();
            RebuildPreview();
        }
    }
}
void EffectEditor::DrawNodeList()
{
    ImGui::Text(u8"【ノード一覧】");

    if (ImGui::Button(u8"+ ノード追加"))
    {
        NodeConfig newNode;
        newNode.id = currentConfig.GenerateNodeId();
        newNode.name = "Node " + std::to_string(currentConfig.nodes.size() + 1);
        newNode.type = RenderType::Particle;
        currentConfig.nodes.push_back(newNode);
        selectedNodeIndex = static_cast<int>(currentConfig.nodes.size() - 1);

        // ノード数が変わるので、プレビューは Update で自動的に作り直される
    }
    ImGui::SameLine();

    // ノードが1個しかない時は削除できないようにする
    if (currentConfig.nodes.size() > 1)
    {
        if (ImGui::Button(u8"- 選択ノード削除"))
        {
            // removedは参照でeraseした後は使えないのでコピーしておく
            const NodeConfig& removed = currentConfig.nodes[selectedNodeIndex];
            int removedId = removed.id;
            int removedParentId = removed.parentId;

            // 消すノードの子を1つ上の親に付け替える
            for (auto& node : currentConfig.nodes)
            {
                if (node.parentId == removedId) node.parentId = removedParentId;
            }

            currentConfig.nodes.erase(currentConfig.nodes.begin() + selectedNodeIndex);
            if (selectedNodeIndex >= currentConfig.nodes.size())
            {
                selectedNodeIndex = static_cast<int>(currentConfig.nodes.size()) - 1;
            }
            // ノード数が変わるので、プレビューは Update で自動的に作り直される
        }
    }
    ImGui::Separator();

    // ノード一覧（親なしのノードから順に、子をツリーで描く）
    for (int i = 0; i < static_cast<int>(currentConfig.nodes.size()); ++i)
    {
        if (currentConfig.nodes[i].parentId == -1)
        {
            DrawNodeTreeItem(i, 0);
        }
    }
    ImGui::Separator();
}
