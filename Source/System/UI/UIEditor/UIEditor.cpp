#include "UIEditor.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/Sprite/Sprite.h"
#include "System/UI/Dialog/Dialog.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"

#include <imgui.h>
#include <fstream>
#include <algorithm>


// 初期化
void UIEditor::Initialize(ID3D11Device* device)
{
	this->device = device;
	uiCanvas.Initialize(this->device);
}

// 更新処理
void UIEditor::Update(float elapsedTime)
{
	uiCanvas.Update(elapsedTime);
}

// 描画処理
void UIEditor::Render(ID3D11DeviceContext* dc)
{
	uiCanvas.Render(dc);
}


// GUI描画
void UIEditor::DrawGUI()
{
	ImGui::Begin(u8"UIエディター");

	DrawToolBar();
	DrawNode();
	DrawInspector();

	ImGui::End();
}
void UIEditor::DrawToolBar()
{
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
			uiCanvas.Save(filepath);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"読み込み"))
	{
		char filepath[MAX_PATH] = { 0 };
		HWND hwnd = Graphics::Instance().GetWindowHandle();
		if (Dialog::OpenFileName(filepath, MAX_PATH,
			"JSON File\0*.json\0\0",
			"ファイルを開く",
			hwnd) == DialogResult::OK)
		{
			uiCanvas.Load(filepath);
			selected = nullptr;
		}
	}
	ImGui::Separator();

	if (ImGui::Button(u8"+ UI追加"))
	{
		selected = AddElement();
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"+ 子を追加"))
	{
		if(selected)
		selected = AddChild(selected);
	}

	// 1個しかないときは削除できないようにする
	if (selected && (selected->parent || uiCanvas.GetElements().size() > 1))
	{
		if (ImGui::Button(u8"- 選択UIを削除"))
		{
			auto& siblings = GetSiblings(selected);

			// selectedと同じものを探す
			auto it = std::find_if(siblings.begin(), siblings.end(),
				[this](const std::unique_ptr<UIElement>& e) {return e.get() == selected; });

			// 見つかったらその要素を消す
			if (it != siblings.end())
			{
				siblings.erase(it);
				selected = nullptr; // 消した要素の場所をnullptrにしておく
			}
		}
	}
	ImGui::Separator();
}
void UIEditor::DrawNode()
{
	// UI一覧
	ImGui::Text(u8"UI一覧");

	// 並び変えボタン

	auto it = std::find_if(uiCanvas.GetElements().begin(), uiCanvas.GetElements().end(),
		[this](const std::unique_ptr<UIElement>& e) {return e.get() == selected; });

	if (ImGui::Button(u8"UIを奥へ"))
	{
		if (selected) MoveElement(selected, -1);
	}
	ImGui::SameLine();

	if (ImGui::Button(u8"UIを手前へ"))
	{
		if (selected) MoveElement(selected, +1);
	}

	// 枠付きの領域
	ImGui::BeginChild("UIList", ImVec2(0, 200), true);
	{
		for (auto& e : uiCanvas.GetElements())
		{
			DrawTreeNode(e.get());
		}
	}
	ImGui::EndChild();
}
void UIEditor::DrawTreeNode(UIElement* e)
{
	ImGui::PushID(e); // ID被り防止

	// 目玉(表示/非表示の切り替え)
	if (ImGui::Button(e->visible ? u8"o" : u8"-", ImVec2(28, 0)))
	{
		e->visible = !e->visible;
	}
	ImGui::SameLine();

	// フラグ
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (e->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;	// 子がいなければ矢印なし
	if (selected == e)       flags |= ImGuiTreeNodeFlags_Selected;	// 選択中ならハイライト

	bool open = ImGui::TreeNodeEx(e, flags, "%s", e->name.c_str());

	// クリックで選択(開閉に関係なくTreeNodeExの直後で見る)
	if (ImGui::IsItemClicked()) selected = e;

	// ツリーを開いていたら子を描く
	if (open)
	{
		for (auto& child : e->children)
		{
			DrawTreeNode(child.get());
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
}
void UIEditor::DrawInspector()
{
	// 選択が可能な時だけ編集UIを描く
	if (selected)
	{
		UIElement& selectedElement = *selected;

		const char* easingItems[] = { "Linear", "EaseInCubic", "EaseOutCubic", "EaseInOutCubic", "EaseOutBack", "EaseOutBounce" };
		const char* loopItems[] = { "Once", "Loop", "PingPong" };

		if (ImGui::BeginTabBar(u8"UITabs"))
		{

			if (ImGui::BeginTabItem(u8"基本"))
			{
				if (ImGui::Button(u8"テクスチャ読み込み"))
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

						SetTexture(selectedElement, pathStr);
					}

				}
				ImGui::SameLine();
				ImGui::Text(u8": %s", selectedElement.texturePath.c_str());
				ImGui::Separator();

				char nameBuffer[128];
				strcpy_s(nameBuffer, selectedElement.name.c_str());
				if (ImGui::InputText(u8"UI名", nameBuffer, sizeof(nameBuffer)))
				{
					selectedElement.name = nameBuffer;
				}
				ImGui::Separator();

				ImGui::DragFloat2(u8"anchor", &selectedElement.anchor.x, 0.01f, 0.0f, 1.0f);
				ImGui::Separator();

				ImGui::DragFloat2(u8"Pivot", &selectedElement.pivot.x, 0.01f, 0.0f, 1.0f);
				ImGui::Separator();

				ImGui::DragFloat2(u8"位置", &selectedElement.position.x, 0.1f);
				ImGui::DragFloat2(u8"画像サイズ", &selectedElement.size.x, 0.1f);
				ImGui::DragFloat4(u8"切り抜き範囲(x,y,幅,高さ)", &selectedElement.uvRect.x, 1.0f);
				ImGui::DragFloat2(u8"拡大率", &selectedElement.scale.x, 0.1f);
				ImGui::DragFloat(u8"回転", &selectedElement.rotation, 0.1f);
				ImGui::ColorEdit4(u8"色", &selectedElement.color.x);
				ImGui::Separator();

				// 並びはFillMethodのenumと必ず同じ順番にする
				const char* fillItems[] = { "None", u8"左から右へ", u8"右から左へ", u8"下から上へ", u8"上から下へ", u8"時計回り", u8"反時計回り" };
				int fillMethod = static_cast<int>(selectedElement.fillMethod);
				if (ImGui::Combo(u8"塗り方向", &fillMethod, fillItems, IM_ARRAYSIZE(fillItems)))
				{
					selectedElement.fillMethod = static_cast<FillMethod>(fillMethod);
				}

				// プレビュー用（保存はしない）
				if (selectedElement.fillMethod != FillMethod::None)
				{
					ImGui::SliderFloat(u8"塗り割合(確認用)", &selectedElement.fillAmount, 0.0f, 1.0f);
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"アニメーション"))
			{
				if (ImGui::Button(u8"リセット"))
				{
					uiCanvas.ResetAnimation(&selectedElement);
				}
				ImGui::Separator();

				ImGui::Checkbox(u8"移動アニメーション", &selectedElement.anim.move.enabled);
				if (selectedElement.anim.move.enabled)
				{
					if (ImGui::Button(u8"テスト##move"))
					{
						selectedElement.anim.move.Start(selectedElement.anim.move.from, selectedElement.anim.move.to, selectedElement.anim.move.duration,
							selectedElement.anim.move.ease,
							selectedElement.anim.move.loop,
							selectedElement.anim.move.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##move"))
					{
						selectedElement.anim.move.Stop();
					}

					int moveLoop = static_cast<int>(selectedElement.anim.move.loop);
					if (ImGui::Combo(u8"ループ##moveLoop", &moveLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.anim.move.loop = static_cast<LoopType>(moveLoop);
					}
					if (selectedElement.anim.move.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.anim.move.loopCount, 1);
					}


					int moveEase = static_cast<int>(selectedElement.anim.move.ease);
					if (ImGui::Combo(u8"イージング##move", &moveEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.anim.move.ease = static_cast<EasingType>(moveEase);
					}

					ImGui::DragFloat(u8"再生時間##move", &selectedElement.anim.move.duration, 0.1f);
					ImGui::Separator();

					ImGui::DragFloat2(u8"開始位置", &selectedElement.anim.move.from.x, 0.1f);
					ImGui::DragFloat2(u8"終了位置", &selectedElement.anim.move.to.x, 0.1f);
					ImGui::Separator();

				}

				ImGui::Checkbox(u8"回転アニメーション", &selectedElement.anim.rotation.enabled);
				if (selectedElement.anim.rotation.enabled)
				{
					if (ImGui::Button(u8"テスト##rotation"))
					{
						selectedElement.anim.rotation.Start(selectedElement.anim.rotation.from, selectedElement.anim.rotation.to, selectedElement.anim.rotation.duration,
							selectedElement.anim.rotation.ease,
							selectedElement.anim.rotation.loop,
							selectedElement.anim.rotation.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##rotation"))
					{
						selectedElement.anim.rotation.Stop();
					}

					int rotationLoop = static_cast<int>(selectedElement.anim.rotation.loop);
					if (ImGui::Combo(u8"ループ##rotation", &rotationLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.anim.rotation.loop = static_cast<LoopType>(rotationLoop);
					}
					if (selectedElement.anim.rotation.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.anim.rotation.loopCount, 1);
					}

					int rotationEase = static_cast<int>(selectedElement.anim.rotation.ease);
					if (ImGui::Combo(u8"イージング##rotation", &rotationEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.anim.rotation.ease = static_cast<EasingType>(rotationEase);
					}

					ImGui::DragFloat(u8"再生時間##rotation", &selectedElement.anim.rotation.duration, 0.1f);
					ImGui::Separator();

					ImGui::DragFloat(u8"開始角度", &selectedElement.anim.rotation.from, 0.1f);
					ImGui::DragFloat(u8"終了角度", &selectedElement.anim.rotation.to, 0.1f);
				}

				ImGui::Checkbox(u8"拡大アニメーション", &selectedElement.anim.scale.enabled);
				if (selectedElement.anim.scale.enabled)
				{
					if (ImGui::Button(u8"テスト##scale"))
					{
						selectedElement.anim.scale.Start(selectedElement.anim.scale.from, selectedElement.anim.scale.to, selectedElement.anim.scale.duration,
							selectedElement.anim.scale.ease,
							selectedElement.anim.scale.loop,
							selectedElement.anim.scale.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##scale"))
					{
						selectedElement.anim.scale.Stop();
					}

					int scaleLoop = static_cast<int>(selectedElement.anim.scale.loop);
					if (ImGui::Combo(u8"ループ##scale", &scaleLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.anim.scale.loop = static_cast<LoopType>(scaleLoop);
					}
					if (selectedElement.anim.scale.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.anim.scale.loopCount, 1);
					}

					int scaleEase = static_cast<int>(selectedElement.anim.scale.ease);
					if (ImGui::Combo(u8"イージング##scale", &scaleEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.anim.scale.ease = static_cast<EasingType>(scaleEase);
					}

					ImGui::DragFloat(u8"再生時間##scale", &selectedElement.anim.scale.duration, 0.1f);
					ImGui::Separator();

					ImGui::DragFloat(u8"開始角度", &selectedElement.anim.scale.from, 0.1f);
					ImGui::DragFloat(u8"終了角度", &selectedElement.anim.scale.to, 0.1f);
				}

				ImGui::Checkbox(u8"フェードアニメーション", &selectedElement.anim.fade.enabled);
				if (selectedElement.anim.fade.enabled)
				{
					if (ImGui::Button(u8"テスト##fade"))
					{
						selectedElement.anim.fade.Start(selectedElement.anim.fade.from, selectedElement.anim.fade.to, selectedElement.anim.fade.duration,
							selectedElement.anim.fade.ease,
							selectedElement.anim.fade.loop,
							selectedElement.anim.fade.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##fade"))
					{
						selectedElement.anim.fade.Stop();
					}

					int fadeLoop = static_cast<int>(selectedElement.anim.fade.loop);
					if (ImGui::Combo(u8"ループ##fadeLoop", &fadeLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.anim.fade.loop = static_cast<LoopType>(fadeLoop);
					}
					if (selectedElement.anim.fade.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.anim.fade.loopCount, 1);
					}

					int fadeEase = static_cast<int>(selectedElement.anim.fade.ease);
					if (ImGui::Combo(u8"イージング##fadeEase", &fadeEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.anim.fade.ease = static_cast<EasingType>(fadeEase);
					}

					ImGui::DragFloat(u8"再生時間", &selectedElement.anim.fade.duration, 0.1f);

					ImGui::DragFloat(u8"開始不透明度", &selectedElement.anim.fade.from, 0.1f);
					ImGui::DragFloat(u8"終了不透明度", &selectedElement.anim.fade.to, 0.1f);

				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"ボタン"))
			{
				bool isButton = (selectedElement.focusIndex >= 0);
				if (ImGui::Checkbox(u8"ボタンとして使う", &isButton))
					selectedElement.focusIndex = isButton ? 0 : -1;

				// ラジオボタンにする場合
				ImGui::Text(u8"並び方（パネル用）");
				if (ImGui::RadioButton(u8"縦", !selectedElement.focusHorizontal)) selectedElement.focusHorizontal = false;
				ImGui::SameLine();
				if (ImGui::RadioButton(u8"横", selectedElement.focusHorizontal)) selectedElement.focusHorizontal = true;
				ImGui::Checkbox(u8"ループ", &selectedElement.focusLoop);
				ImGui::DragInt(u8"選択番号", &selectedElement.focusIndex, 1, -1, 99);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"ハイライト"))
			{
				if (ImGui::Button(u8"マスク読み込み"))
				{
					char filePath[MAX_PATH] = { 0 };
					HWND hwnd = Graphics::Instance().GetWindowHandle();
					if (Dialog::OpenFileName(filePath, MAX_PATH,
						"PNG File\0*.png\0All Files\0*.*\0\0", "マスク選択", hwnd) == DialogResult::OK)
					{
						std::string pathStr = filePath;
						size_t pos = pathStr.find("Data");
						if (pos != std::string::npos) pathStr = pathStr.substr(pos);

						selectedElement.maskTexturePath = pathStr;
						selectedElement.maskSRV = TextureResourceManager::Instance().GetOrLoad(device, pathStr);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button(u8"外す"))
				{
					selectedElement.maskTexturePath.clear();
					selectedElement.maskSRV.Reset();
				}
				ImGui::Text(u8": %s", selectedElement.maskTexturePath.c_str());

				if (selectedElement.maskSRV)
				{
					// 色(rgb)と強さ(a)。強さは1を超えてもいいので別のスライダーにする
					ImGui::ColorEdit3(u8"光の色", &selectedElement.glowColor.x);
					ImGui::DragFloat(u8"光の強さ", &selectedElement.glowColor.w, 0.05f, 0.0f, 5.0f);

					// 確認用（保存しない）：エディタでは選択されないので、手で光らせて見る
					ImGui::SliderFloat(u8"ハイライト(確認用)", &selectedElement.highlightTarget, 0.0f, 1.0f);
				}

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	else
	{
		ImGui::TextDisabled(u8"UIが選択されていません");
	}
}


// UI追加
UIElement* UIEditor::AddElement()
{
	auto e = std::make_unique<UIElement>();
	e->name = "Element" + std::to_string(uiCanvas.GetElements().size() + 1);
	e->position = { 0.0f, 0.0f }; // 画面中央

	// オブジェクトの所有権を移動させて末尾に追加する
	uiCanvas.GetElements().emplace_back(std::move(e));
	return uiCanvas.GetElements().back().get();
}
UIElement* UIEditor::AddChild(UIElement* parent)
{
	auto e = std::make_unique<UIElement>();
	e->parent = parent;
	e->name = "child" + std::to_string(parent->children.size() + 1);
	e->position = { 0.0f, 0.0f };
	parent->children.emplace_back(std::move(e));
	return parent->children.back().get();
}

// テクスチャ設定
void UIEditor::SetTexture(UIElement& e, const std::string& path)
{
	e.texturePath = path;

	// キャッシュ済みならそれを使い回す
	auto srv = TextureResourceManager::Instance().GetOrLoad(device, path);
	e.sprite = uiCanvas.CreateSprite(path);

	DirectX::XMFLOAT2 texSize = e.sprite->GetTextureSize();
	e.size = texSize; // 画像のサイズで表示する
}

// Elementの移動
void UIEditor::MoveElement(UIElement* e, int direction)
{
	auto& siblings = GetSiblings(e);

	// selectedと同じものを探す
	auto it = std::find_if(siblings.begin(), siblings.end(),
		[e](const std::unique_ptr<UIElement>& s) {return s.get() == e; });

	int from = static_cast<int>(it - siblings.begin()); // 移動元
	int to = from + direction; // 移動先
	if (to < 0 || to >= static_cast<int>(siblings.size()))return;

	// 中身を取り出す(取り出した後のelemens[from]は空になる)
	auto moved = std::move(siblings[from]);

	// 空になった箱を配列から取り除く
	siblings.erase(siblings.begin() + from);

	// 目的の位置に入れ直す
	siblings.insert(siblings.begin() + to, std::move(moved));
}

// 要素が入っている配列(兄弟の配列)を返す
std::vector<std::unique_ptr<UIElement>>& UIEditor::GetSiblings(UIElement* e)
{
	if (e->parent)
		return e->parent->children; // 親がいれば親の子の配列
	else
		return uiCanvas.GetElements(); // いなければルートの配列
}

