#include "DefenseMagic.h"

#include "GamePlay/Object/Character/Character.h"

#include "System/Core/Input/Input.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"

#include "json.hpp"

#include <imgui.h>
#include <fstream>

// 初期化
void DefenseMagic::Initialize(Character* owner)
{
	// 所有者を受け取る
	this->owner = owner;

	// 防御魔法のJsonファイルを読み込む
	Load();

	// エフェクトをロードする
}

// 更新処理
void DefenseMagic::Update(Character* owner, float elapsedTime)
{
	if (isActive)
	{
		// 防御魔法を出した位置に展開する
		DirectX::XMFLOAT3 playerPos = owner->GetPosition();
		defensePosition.x = playerPos.x + defensePositionOffset.x;
		defensePosition.y = playerPos.y + defensePositionOffset.y;
		defensePosition.z = playerPos.z + defensePositionOffset.z;
	}
}

// 描画処理
void DefenseMagic::Render()
{
	// ここでエフェクトを描画する
	if (isActive)
	{

	}
}

void DefenseMagic::Save()
{
	nlohmann::json j;
	j["defensePositionOffset"] = { defensePositionOffset.x, defensePositionOffset.y, defensePositionOffset.z };
	j["defenseRadius"] = defenseRadius;

	std::ofstream file("Data/Json/Magic/DefenseMagic.json");
	file << j.dump(4);
}

void DefenseMagic::Load()
{
	std::ifstream file("Data/Json/Magic/DefenseMagic.json");

	if (file.is_open())
	{
		nlohmann::json j;
		file >> j;

		if (j.find("defensePositionOffset") != j.end()) defensePositionOffset = { j["defensePositionOffset"][0], j["defensePositionOffset"][1], j["defensePositionOffset"][2] };
		if (j.find("defenseRadius") != j.end()) defenseRadius = j["defenseRadius"];
	}
}

// GUI描画
void DefenseMagic::DrawGUI()
{
	if (ImGui::CollapsingHeader(u8"防御魔法", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 保存
		if (ImGui::Button(u8"保存##Defense"))
		{
			Save();
		}

		ImGui::SameLine();

		// 読み込み
		if (ImGui::Button(u8"読み込み##Defense"))
		{
			Load();
		}
		ImGui::Separator();

		ImGui::Checkbox(u8"防御魔法展開", &isActive);

		ImGui::DragFloat3(u8"防御魔法のオフセット位置", &defensePositionOffset.x, 0.1f);
		ImGui::DragFloat(u8"防御魔法の半径", &defenseRadius, 0.1f);
	}
}