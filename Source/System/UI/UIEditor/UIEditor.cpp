#include "UIEditor.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/Sprite/Sprite.h"
#include "System/UI/Dialog/Dialog.h"
#include "json.hpp"

#include <imgui.h>
#include <fstream>


// 初期化
void UIEditor::Initialize(ID3D11Device* device)
{
	this->device = device;

	element.texturePath = "Data/Sprite/Texture/aruki.png";
	element.position = { 960.0f, 540.0f };
	element.size = { 200.0f, 200.0f };

	// スプライトを作る
	sprite = std::make_shared<Sprite>(device, element.texturePath.c_str());
}

// 更新処理
void UIEditor::Update(float elapsedTime)
{

}

// 描画処理
void UIEditor::Render(ID3D11DeviceContext* dc)
{
	// 表示サイズ
	float w = element.size.x * element.scale.x;
	float h = element.size.y * element.scale.y;
	float x = element.position.x;
	float y = element.position.y;

	// 4隅(左上、右上、左下、右下)
	DirectX::XMFLOAT2 cornners[4] = {
		{x, y},
		{x + w, y},
		{x, y + h},
		{x + w, y + h}
	};

	// 切り抜き範囲（sw,shが0なら画像全体を使う）
	DirectX::XMFLOAT4 uv = element.uvRect;
	if (uv.z <= 0.0f || uv.w <= 0.0f);
	{
		DirectX::XMFLOAT2 texSize = sprite->GetTextureSize();
		uv = { 0.0f, 0.0f, texSize.x, texSize.y };
	}

	sprite->Render(dc,
		cornners,
		0.0f,
		uv.x, uv.y, uv.z, uv.w,
		element.color);
}

// GUI描画
void UIEditor::DrawGUI()
{
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

	char nameBuffer[128];
	strcpy_s(nameBuffer, element.name.c_str());
	if (ImGui::InputText(u8"UI名", nameBuffer, sizeof(nameBuffer)))
	{
		element.name = nameBuffer;
	}

	ImGui::DragFloat2(u8"位置", &element.position.x, 0.1f);
	ImGui::DragFloat2(u8"画像サイズ", &element.size.x, 0.1f);
	ImGui::DragFloat2(u8"拡大率", &element.scale.x, 0.1f);
	ImGui::DragFloat(u8"回転", &element.rotation, 0.1f);
	ImGui::ColorEdit4(u8"色", &element.color.x);

	ImGui::End();
}

// 保存
void UIEditor::Save(const std::string& path)
{
	using json = nlohmann::json;
	json j;
	j["name"] = element.name;
	j["texturePath"] = element.texturePath;
	j["uvRect"] = { element.uvRect.x, element.uvRect.y, element.uvRect.z, element.uvRect.w };

	j["position"] = { element.position.x, element.position.y };
	j["rotation"] = element.rotation;
	j["size"] = { element.size.x, element.size.y };
	j["scale"] = { element.scale.x, element.scale.y };
	j["pivot"] = { element.pivot.x, element.pivot.y };
	j["anchor"] = { element.anchor.x, element.anchor.y };
	j["color"] = { element.color.x, element.color.y, element.color.z, element.color.w };

	j["visible"] = element.visible;

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

	element.name = j.value("name", "");
	element.texturePath = j.value("texturePath", "");
	if (j.find("uvRect") != j.end()) element.uvRect = { j["uvRect"][0], j["uvRect"][1] , j["uvRect"][2] , j["uvRect"][3] };


	if (j.find("position") != j.end()) element.position = { j["position"][0], j["position"][1] };
	element.rotation = j.value("rotation", 0.0f);
	if (j.find("size") != j.end()) element.size = { j["size"][0], j["size"][1] };
	if (j.find("scale") != j.end()) element.scale = { j["scale"][0], j["scale"][1] };
	if (j.find("pivot") != j.end()) element.pivot = { j["pivot"][0], j["pivot"][1] };
	if (j.find("anchor") != j.end()) element.anchor = { j["anchor"][0], j["anchor"][1] };

	if (j.find("color") != j.end()) element.color = { j["color"][0], j["color"][1] , j["color"][2] , j["color"][3] };
	element.visible = j.value("visible", false);
}