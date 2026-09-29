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
				ImGui::DragFloat2(u8"拡大率", &selectedElement.scale.x, 0.1f);
				ImGui::DragFloat(u8"回転", &selectedElement.rotation, 0.1f);
				ImGui::ColorEdit4(u8"色", &selectedElement.color.x);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(u8"アニメーション"))
			{
				if (ImGui::Button(u8"リセット"))
				{
					uiCanvas.ResetAnimation(&selectedElement);
				}
				ImGui::Separator();

				ImGui::Checkbox(u8"移動アニメーション", &selectedElement.moveAnim.enabled);
				if (selectedElement.moveAnim.enabled)
				{
					if (ImGui::Button(u8"テスト##move"))
					{
						selectedElement.moveAnim.Start(selectedElement.moveAnim.from, selectedElement.moveAnim.to, selectedElement.moveAnim.duration,
							selectedElement.moveAnim.ease,
							selectedElement.moveAnim.loop,
							selectedElement.moveAnim.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##move"))
					{
						selectedElement.moveAnim.Stop();
					}

					int moveLoop = static_cast<int>(selectedElement.moveAnim.loop);
					if (ImGui::Combo(u8"ループ##moveLoop", &moveLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.moveAnim.loop = static_cast<LoopType>(moveLoop);
					}
					if (selectedElement.moveAnim.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.moveAnim.loopCount, 1);
					}


					int moveEase = static_cast<int>(selectedElement.moveAnim.ease);
					if (ImGui::Combo(u8"イージング##move", &moveEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.moveAnim.ease = static_cast<EasingType>(moveEase);
					}

					ImGui::DragFloat(u8"再生時間##move", &selectedElement.moveAnim.duration, 0.1f);
					ImGui::Separator();

					ImGui::DragFloat2(u8"開始位置", &selectedElement.moveAnim.from.x, 0.1f);
					ImGui::DragFloat2(u8"終了位置", &selectedElement.moveAnim.to.x, 0.1f);
					ImGui::Separator();

				}

				ImGui::Checkbox(u8"回転アニメーション", &selectedElement.rotationAnim.enabled);
				if (selectedElement.rotationAnim.enabled)
				{
					if (ImGui::Button(u8"テスト##rotation"))
					{
						selectedElement.rotationAnim.Start(selectedElement.rotationAnim.from, selectedElement.rotationAnim.to, selectedElement.rotationAnim.duration,
							selectedElement.rotationAnim.ease,
							selectedElement.rotationAnim.loop,
							selectedElement.rotationAnim.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##rotation"))
					{
						selectedElement.rotationAnim.Stop();
					}

					int rotationLoop = static_cast<int>(selectedElement.rotationAnim.loop);
					if (ImGui::Combo(u8"ループ##rotation", &rotationLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.rotationAnim.loop = static_cast<LoopType>(rotationLoop);
					}
					if (selectedElement.rotationAnim.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.rotationAnim.loopCount, 1);
					}

					int rotationEase = static_cast<int>(selectedElement.rotationAnim.ease);
					if (ImGui::Combo(u8"イージング##rotation", &rotationEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.rotationAnim.ease = static_cast<EasingType>(rotationEase);
					}

					ImGui::DragFloat(u8"再生時間##rotation", &selectedElement.rotationAnim.duration, 0.1f);
					ImGui::Separator();

					ImGui::DragFloat(u8"開始角度", &selectedElement.rotationAnim.from, 0.1f);
					ImGui::DragFloat(u8"終了角度", &selectedElement.rotationAnim.to, 0.1f);
				}

				ImGui::Checkbox(u8"拡大アニメーション", &selectedElement.scaleAnim.enabled);
				if (selectedElement.scaleAnim.enabled)
				{
					if (ImGui::Button(u8"テスト##scale"))
					{
						selectedElement.scaleAnim.Start(selectedElement.scaleAnim.from, selectedElement.scaleAnim.to, selectedElement.scaleAnim.duration,
							selectedElement.scaleAnim.ease,
							selectedElement.scaleAnim.loop,
							selectedElement.scaleAnim.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##scale"))
					{
						selectedElement.scaleAnim.Stop();
					}

					int scaleLoop = static_cast<int>(selectedElement.scaleAnim.loop);
					if (ImGui::Combo(u8"ループ##scale", &scaleLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.scaleAnim.loop = static_cast<LoopType>(scaleLoop);
					}
					if (selectedElement.scaleAnim.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.scaleAnim.loopCount, 1);
					}

					int scaleEase = static_cast<int>(selectedElement.scaleAnim.ease);
					if (ImGui::Combo(u8"イージング##scale", &scaleEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.scaleAnim.ease = static_cast<EasingType>(scaleEase);
					}

					ImGui::DragFloat(u8"再生時間##scale", &selectedElement.scaleAnim.duration, 0.1f);
					ImGui::Separator();

					ImGui::DragFloat(u8"開始角度", &selectedElement.scaleAnim.from, 0.1f);
					ImGui::DragFloat(u8"終了角度", &selectedElement.scaleAnim.to, 0.1f);
				}

				ImGui::Checkbox(u8"フェードアニメーション", &selectedElement.fadeAnim.enabled);
				if (selectedElement.fadeAnim.enabled)
				{
					if (ImGui::Button(u8"テスト##fade"))
					{
						selectedElement.fadeAnim.Start(selectedElement.fadeAnim.from, selectedElement.fadeAnim.to, selectedElement.fadeAnim.duration,
							selectedElement.fadeAnim.ease,
							selectedElement.fadeAnim.loop,
							selectedElement.fadeAnim.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##fade"))
					{
						selectedElement.fadeAnim.Stop();
					}

					int fadeLoop = static_cast<int>(selectedElement.fadeAnim.loop);
					if (ImGui::Combo(u8"ループ##fadeLoop", &fadeLoop, loopItems, IM_ARRAYSIZE(loopItems)))
					{
						selectedElement.fadeAnim.loop = static_cast<LoopType>(fadeLoop);
					}
					if (selectedElement.fadeAnim.loop == LoopType::PingPong)
					{
						ImGui::DragInt(u8"往復回数", &selectedElement.fadeAnim.loopCount, 1);
					}

					int fadeEase = static_cast<int>(selectedElement.fadeAnim.ease);
					if (ImGui::Combo(u8"イージング##fadeEase", &fadeEase, easingItems, IM_ARRAYSIZE(easingItems)))
					{
						selectedElement.fadeAnim.ease = static_cast<EasingType>(fadeEase);
					}

					ImGui::DragFloat(u8"再生時間", &selectedElement.fadeAnim.duration, 0.1f);

					ImGui::DragFloat(u8"開始不透明度", &selectedElement.fadeAnim.from, 0.1f);
					ImGui::DragFloat(u8"終了不透明度", &selectedElement.fadeAnim.to, 0.1f);

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
	e.sprite = std::make_shared<Sprite>(device, srv);

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
	uiCanvas.GetElements().erase(siblings.begin() + from);

	// 目的の位置に入れ直す
	uiCanvas.GetElements().insert(siblings.begin() + to, std::move(moved));
}

// 要素が入っている配列(兄弟の配列)を返す
std::vector<std::unique_ptr<UIElement>>& UIEditor::GetSiblings(UIElement* e)
{
	if (e->parent)
		return e->parent->children; // 親がいれば親の子の配列
	else
		return uiCanvas.GetElements(); // いなければルートの配列
}

