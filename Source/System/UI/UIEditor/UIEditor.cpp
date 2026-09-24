#include "UIEditor.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/Sprite/Sprite.h"
#include "System/UI/Dialog/Dialog.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"
#include "json.hpp"

#include <imgui.h>
#include <fstream>


// 初期化
void UIEditor::Initialize(ID3D11Device* device)
{
	this->device = device;
}

// 更新処理
void UIEditor::Update(float elapsedTime)
{

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

	for (auto& e : elements)
	{
		if (!e->visible || e->texturePath.empty() || !e->sprite) continue;

		// 表示サイズ
		float w = e->size.x * e->scale.x;
		float h = e->size.y * e->scale.y;

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

		// 回転→位置に移動
		float theta = DirectX::XMConvertToRadians(e->rotation);
		float c = cosf(theta);
		float s = sinf(theta);
		for (auto& p : cornners)
		{
			float rx = c * p.x - s * p.y;
			float ry = s * p.x + c * p.y;
			p.x = rx + e->position.x;
			p.y = ry + e->position.y;
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
			e->color);
	}
}

// GUI描画
void UIEditor::DrawGUI()
{
	if (selectedIndex >= elements.size())
	{
		selectedIndex = 0;
	}

	ImGui::Begin(u8"UIエディター");

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
		AddElement();
	}
	ImGui::SameLine();

	// 1個しかないときは削除できないようにする
	if (elements.size() > 1)
	{
		if (ImGui::Button(u8"- 選択UIを削除"))
		{
			elements.erase(elements.begin() + selectedIndex);
			if (selectedIndex >= 0 && selectedIndex < static_cast<int>(elements.size()))
			{
				selectedIndex = static_cast<int>(elements.size()) - 1;
			}
		}
	}
	ImGui::Separator();


	// UI一覧
	for (int i = 0; i < elements.size(); i++)
	{
		auto& e = elements[i];
		bool isSelected = (selectedIndex == i);

		ImGui::PushID(i); // ID被り防止

		if (ImGui::Selectable(e->name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
		{
			selectedIndex = i;
		}
		ImGui::PopID();
	}
	ImGui::Separator();

	// 選択が可能な時だけ編集UIを描く
	if (selectedIndex >= 0 && selectedIndex < static_cast<int>(elements.size()))
	{
		UIElement& selectedElement = *elements[selectedIndex];

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


		ImGui::Checkbox(selectedElement.visible ? u8"表示" : u8"非表示", &selectedElement.visible);
		char nameBuffer[128];
		strcpy_s(nameBuffer, selectedElement.name.c_str());
		if (ImGui::InputText(u8"UI名", nameBuffer, sizeof(nameBuffer)))
		{
			selectedElement.name = nameBuffer;
		}
		ImGui::Separator();


		ImGui::DragFloat2(u8"基準点", &selectedElement.pivot.x, 0.01f, 0.0f, 1.0f);
		ImGui::Separator();

		ImGui::DragFloat2(u8"位置", &selectedElement.position.x, 0.1f);
		ImGui::DragFloat2(u8"画像サイズ", &selectedElement.size.x, 0.1f);
		ImGui::DragFloat2(u8"拡大率", &selectedElement.scale.x, 0.1f);
		ImGui::DragFloat(u8"回転", &selectedElement.rotation, 0.1f);
		ImGui::ColorEdit4(u8"色", &selectedElement.color.x);

	}
	else
	{
		ImGui::TextDisabled(u8"UIが選択されていません");
	}

	ImGui::End();
}

// 保存
void UIEditor::Save(const std::string& path)
{
	using json = nlohmann::json;
	json j;
	j["UI"] = json::array();

	for (auto& e : elements)
	{
		json element;
		element["name"] = e->name;
		element["texturePath"] = e->texturePath;
		element["uvRect"] = { e->uvRect.x, e->uvRect.y, e->uvRect.z, e->uvRect.w };

		element["position"] = { e->position.x, e->position.y };
		element["rotation"] = e->rotation;
		element["size"] = { e->size.x, e->size.y };
		element["scale"] = { e->scale.x, e->scale.y };
		element["pivot"] = { e->pivot.x, e->pivot.y };
		element["anchor"] = { e->anchor.x, e->anchor.y };
		element["color"] = { e->color.x, e->color.y, e->color.z, e->color.w };

		element["visible"] = e->visible;

		j["UI"].push_back(element);
	}

	std::ofstream file(path);
	if (file.is_open())
	{
		file << j.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
	}
}

// 読み込み
void UIEditor::Load(const std::string& path)
{
	using json = nlohmann::json;
	std::ifstream file(path);
	if (!file.is_open()) return;

	json j;
	file >> j;

	// 配列が存在する場合全て読み込む
	if (j.find("UI") != j.end() && j["UI"].is_array())
	{
		// 既存のElementを削除してリセット！
		elements.clear();

		for (const auto& je : j["UI"])   // je = 要素1個分のJSON
		{
			auto element = std::make_unique<UIElement>();

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

			elements.emplace_back(std::move(element));   // ← 配列に入れる
		}

		selectedIndex = elements.empty() ? -1 : 0;   // ← 選択も戻す
	}
}

// UI追加
UIElement* UIEditor::AddElement()
{
	auto e = std::make_unique<UIElement>();
	e->name = "Element" + std::to_string(elements.size() + 1);
	e->position = { 960.0f, 540.0f }; // 画面中央

	// オブジェクトの所有権を移動させて末尾に追加する
	elements.emplace_back(std::move(e));
	selectedIndex = static_cast<int>(elements.size() - 1);
	return elements.back().get();
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
