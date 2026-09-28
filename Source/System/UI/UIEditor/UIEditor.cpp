#include "UIEditor.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/Sprite/Sprite.h"
#include "System/UI/Dialog/Dialog.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"

#include <imgui.h>
#include <fstream>
#include <algorithm>


static nlohmann::json ToJson(float v) { return v; }
static nlohmann::json ToJson(const DirectX::XMFLOAT2& v) { return { v.x, v.y }; }

static void FromJson(const nlohmann::json& j, float& v) { v = j.get<float>(); }
static void FromJson(const nlohmann::json& j, DirectX::XMFLOAT2& v) { v = { j[0], j[1] }; }

template<typename T>
static nlohmann::json SaveTween(const UITween<T>& tween)
{
	return {
		{"from", ToJson(tween.from)},
		{"to", ToJson(tween.to)},
		{"duration", tween.duration},
		{"ease", static_cast<int>(tween.ease)},
		{"loop", static_cast<int>(tween.loop)},
		{"loopCount", tween.loopCount},
		{"enabled", tween.enabled},
	};
}

// Tween1個分の設定を読み込み(ない項目は今の値のまま)
template<typename T>
static void LoadTween(const nlohmann::json& j, UITween<T>& tween)
{
	if (j.find("from") != j.end())      FromJson(j["from"], tween.from);
	if (j.find("to") != j.end())        FromJson(j["to"], tween.to);
	tween.duration = j.value("duration", tween.duration);
	tween.ease = static_cast<EasingType>(j.value("ease", static_cast<int>(tween.ease)));
	tween.loop = static_cast<LoopType>(j.value("loop", static_cast<int>(tween.loop)));
	tween.loopCount = j.value("loopCount", -1);
	tween.enabled = j.value("enabled", false);
}

// 初期化
void UIEditor::Initialize(ID3D11Device* device)
{
	this->device = device;
}

// 更新処理
void UIEditor::Update(float elapsedTime)
{
	for (auto& e : canvas.elements)
	{
		UpdateElement(e.get(), elapsedTime);
	}
}
void UIEditor::UpdateElement(UIElement* e, float elapsedTime)
{
	// 自分自身を更新
	if (e->fadeAnim.playing)
	{
		e->fadeAnim.Update(elapsedTime);
		e->fadeAlpha = e->fadeAnim.GetValue();
	}
	
	if (e->moveAnim.playing)
	{
		e->moveAnim.Update(elapsedTime);
		e->animPosition = e->moveAnim.GetValue();
	}

	if (e->rotationAnim.playing)
	{
		e->rotationAnim.Update(elapsedTime);
		e->rotationValue = e->rotationAnim.GetValue();
	}

	if (e->scaleAnim.playing)
	{
		e->scaleAnim.Update(elapsedTime);
		e->scaleValue = e->scaleAnim.GetValue();
	}

	// 子を更新する
	for (auto& child : e->children)
	{
		UpdateElement(child.get(), elapsedTime);
	}
}

// 描画処理
void UIEditor::Render(ID3D11DeviceContext* dc)
{
	RenderState* renderState = Graphics::Instance().GetRenderState();

	// 半透明で描く
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	// 深度は見ないので書かない(常に配列の順番通りに描く)
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	// <テクスチャのサンプラーをセットする
	ID3D11SamplerState* sampler = renderState->GetSamplerState(SamplerState::LinearClamp);
	dc->PSSetSamplers(0, 1, &sampler);

	for (auto& e : canvas.elements)
	{
		// eは今処理をしている自分
		RenderElement(dc, e.get(), DirectX::XMMatrixIdentity(), {1, 1, 1, 1});
	}
}
void UIEditor::RenderElement(ID3D11DeviceContext* dc, UIElement* e, const DirectX::XMMATRIX& parentWorld, const DirectX::XMFLOAT4& parentColor)
{
	// 非表示なら子も含めて描かない
	if (!e->visible) return;

	// 親の四角形の中のanhcorの位置
	DirectX::XMFLOAT2 anchorPos;
	if (e->parent)
	{
		// 親の(0,0)は親のpivotの位置なので、pivotからの距離にする
		anchorPos.x = (e->anchor.x - e->parent->pivot.x) * e->parent->size.x;
		anchorPos.y = (e->anchor.y - e->parent->pivot.y) * e->parent->size.y;
	}
	else
	{
		// ルートの親はCanvas(左上(0,0))
		anchorPos.x = e->anchor.x * canvas.referenceResolution.x;
		anchorPos.y = e->anchor.y * canvas.referenceResolution.y;
	}

	// ローカル行列を作る
	DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(e->scale.x * e->scaleValue, e->scale.y * e->scaleValue, 1.0f);
	DirectX::XMMATRIX Angle = DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(e->rotation + e->rotationValue)); // Z軸しか2Dは回転しやんからXMMatrixRotationZ

	DirectX::XMFLOAT2 pos = e->moveAnim.enabled ? e->animPosition : e->position;
	DirectX::XMMATRIX Position = DirectX::XMMatrixTranslation(
		anchorPos.x + pos.x,
		anchorPos.y + pos.y,
		0.0f
	);
	DirectX::XMMATRIX world = Scale * Angle * Position * parentWorld;
	DirectX::XMStoreFloat4x4(&e->worldMatrix, world);

	e->worldColor = {
	parentColor.x * e->color.x,
	parentColor.y * e->color.y,
	parentColor.z * e->color.z,
	parentColor.w * e->color.w * e->fadeAlpha
	};

	if (!e->texturePath.empty() && e->sprite)
	{
		// 表示サイズ
		float w = e->size.x;
		float h = e->size.y;

		// 基準点(画像の中のどこか, px)
		// pivot×表示サイズ
		float px = e->pivot.x * w;
		float py = e->pivot.y * h;

		// 基準点を原点にした4隅(左上、右上、左下、右下)
		DirectX::XMFLOAT2 cornners[4] = {
			{-px, -py},
			{w - px, -py},
			{-px, h - py},
			{w - px, h - py}
		};

		// 4隅をワールド行列で変換（回転・拡大・移動が全部かかる）
		for (auto& p : cornners)
		{
			DirectX::XMVECTOR v = DirectX::XMLoadFloat2(&p);
			v = DirectX::XMVector2TransformCoord(v, world);
			DirectX::XMStoreFloat2(&p, v);
		}

		// 切り抜き範囲（sw,shが0なら画像全体を使う）
		DirectX::XMFLOAT4 uv = e->uvRect;
		if (uv.z <= 0.0f || uv.w <= 0.0f)
		{
			DirectX::XMFLOAT2 texSize = e->sprite->GetTextureSize();
			uv = { 0.0f, 0.0f, texSize.x, texSize.y };
		}

		e->sprite->Render(dc,
			cornners,
			0.0f,
			uv.x, uv.y, uv.z, uv.w,
			e->worldColor);
	}

	// 子を描く(自分の後に描くので子が手前に出る)
	for (auto& child : e->children)
	{
		RenderElement(dc, child.get(), world, e->worldColor);
	}
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
			Save(filepath);
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
			Load(filepath);
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
	if (selected && (selected->parent || canvas.elements.size() > 1))
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

	auto it = std::find_if(canvas.elements.begin(), canvas.elements.end(),
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
		for (auto& e : canvas.elements)
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
					ResetAnimation(&selectedElement);
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

					if (ImGui::Button(u8"フェードイン再生"))
					{
						selectedElement.fadeAnim.Start(0.0f, 1.0f, selectedElement.fadeAnim.duration,
							selectedElement.fadeAnim.ease,
							selectedElement.fadeAnim.loop,
							selectedElement.fadeAnim.loopCount);
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"フェードアウト再生"))
					{
						selectedElement.fadeAnim.Start(1.0f, 0.0f, selectedElement.fadeAnim.duration,
							selectedElement.fadeAnim.ease,
							selectedElement.fadeAnim.loop,
							selectedElement.fadeAnim.loopCount);
					}
					ImGui::Separator();

					if (ImGui::Button(u8"リセット"))
					{
						selectedElement.fadeAnim.playing = false;
						selectedElement.fadeAlpha = 1.0f;
					}
					ImGui::SameLine();
					if (ImGui::Button(u8"停止##scale"))
					{
						selectedElement.fadeAnim.Stop();
					}
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


// 保存
void UIEditor::Save(const std::string& path)
{
	using json = nlohmann::json;
	json j;

	j["referenceResolution"] = { canvas.referenceResolution.x, canvas.referenceResolution.y };

	// 要素
	j["elements"] = json::array();
	for (auto& e : canvas.elements)
	{
		j["elements"] .push_back(SaveElement(e.get()));
	}

	std::ofstream file(path);
	if (file.is_open())
	{
		file << j.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
	}
}
nlohmann::json UIEditor::SaveElement(const UIElement* e)
{
	using json = nlohmann::json;

	// 自分の中身
	json j = {
			{"name",e->name},
			{"texturePath", e->texturePath},
			{"uvRect", { e->uvRect.x, e->uvRect.y, e->uvRect.z, e->uvRect.w }},
			{"position", {e->position.x, e->position.y}},
			{"rotation", e->rotation},
			{"size", {e->size.x, e->size.y}},
			{"scale", {e->scale.x, e->scale.y}},
			{"pivot", {e->pivot.x, e->pivot.y}},
			{"anchor", {e->anchor.x, e->anchor.y}},
			{"color", { e->color.x, e->color.y, e->color.z, e->color.w }},
			{"visible", e->visible},

	};

	j["animations"] = {
	{"fade", SaveTween(e->fadeAnim)},
	{"move", SaveTween(e->moveAnim)},
	{"rotation", SaveTween(e->rotationAnim)},
	{"scale", SaveTween(e->scaleAnim)},
	};

	// 子は子自身に保存してもらう
	j["children"] = json::array();
	for (auto& child : e->children)
	{
		j["children"].push_back(SaveElement(child.get()));
	}

	return j;
}

// 読み込み
void UIEditor::Load(const std::string& path)
{
	using json = nlohmann::json;
	std::ifstream file(path);
	if (!file.is_open()) return;

	json j;
	file >> j;

	if (j.find("referenceResolution") != j.end()) canvas.referenceResolution = { j["referenceResolution"][0], j["referenceResolution"][1] };

	// 配列が存在する場合全て読み込む
	if (j.find("elements") != j.end() && j["elements"].is_array())
	{
		// 既存のElementを削除してリセット！
		canvas.elements.clear();
		selected = nullptr;

		for (const auto& je : j["elements"])   // je = 要素1個分のJSON
		{
			// ルートなので親はいない
			canvas.elements.emplace_back(LoadElement(je, nullptr));
		}
	}
}
std::unique_ptr<UIElement> UIEditor::LoadElement(const nlohmann::json& je, UIElement* parent)
{
	auto element = std::make_unique<UIElement>();
	element->parent = parent;

	// 自分自身の中身
	element->name = je.value("name", "");
	element->texturePath = je.value("texturePath", "");
	if (je.find("uvRect") != je.end())   element->uvRect = { je["uvRect"][0], je["uvRect"][1], je["uvRect"][2], je["uvRect"][3] };
	if (je.find("position") != je.end()) element->position = { je["position"][0], je["position"][1] };
	element->rotation = je.value("rotation", 0.0f);
	if (je.find("size") != je.end())     element->size = { je["size"][0], je["size"][1] };
	if (je.find("scale") != je.end())    element->scale = { je["scale"][0], je["scale"][1] };
	if (je.find("pivot") != je.end())    element->pivot = { je["pivot"][0], je["pivot"][1] };
	if (je.find("anchor") != je.end())   element->anchor = { je["anchor"][0], je["anchor"][1] };
	if (je.find("color") != je.end())    element->color = { je["color"][0], je["color"][1], je["color"][2], je["color"][3] };
	element->visible = je.value("visible", true);   // ← 初期値は true に

	// テクスチャを読み直す
	if (!element->texturePath.empty())
	{
		element->sprite = std::make_shared<Sprite>(device, element->texturePath.c_str());
	}

	if (je.find("animations") != je.end())
	{
		const auto& ja = je["animations"];
		if (ja.find("fade") != ja.end())     LoadTween(ja["fade"], element->fadeAnim);
		if (ja.find("move") != ja.end())     LoadTween(ja["move"], element->moveAnim);
		if (ja.find("rotation") != ja.end()) LoadTween(ja["rotation"], element->rotationAnim);
		if (ja.find("scale") != ja.end())    LoadTween(ja["scale"], element->scaleAnim);
	}

	// 子は子自身に読み込んでもらう
	if (je.find("children") != je.end() && je["children"].is_array())
	{
		for (const auto& jc : je["children"])
		{
			element->children.emplace_back(LoadElement(jc, element.get()));
		}
	}

	return element;
}

// UI追加
UIElement* UIEditor::AddElement()
{
	auto e = std::make_unique<UIElement>();
	e->name = "Element" + std::to_string(canvas.elements.size() + 1);
	e->position = { 0.0f, 0.0f }; // 画面中央

	// オブジェクトの所有権を移動させて末尾に追加する
	canvas.elements.emplace_back(std::move(e));
	return canvas.elements.back().get();
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
	canvas.elements.erase(siblings.begin() + from);

	// 目的の位置に入れ直す
	canvas.elements.insert(siblings.begin() + to, std::move(moved));
}

// 要素が入っている配列(兄弟の配列)を返す
std::vector<std::unique_ptr<UIElement>>& UIEditor::GetSiblings(UIElement* e)
{
	if (e->parent)
		return e->parent->children; // 親がいれば親の子の配列
	else
		return canvas.elements; // いなければルートの配列
}

// 要素のアニメーションを全部止めて、見た目を元に戻す
void UIEditor::ResetAnimation(UIElement* e)
{
	// 全部のTweenを止める(from・to・時間などの設定は残る)
	e->fadeAnim.Stop();
	e->moveAnim.Stop();
	e->rotationAnim.Stop();
	e->scaleAnim.Stop();

	// アニメーションの結果を初期値に戻す
	e->fadeAlpha = 1.0f;
	e->animPosition = { 0.0f, 0.0f };
	e->rotationValue = 0.0f;
	e->scaleValue = 1.0f;
}