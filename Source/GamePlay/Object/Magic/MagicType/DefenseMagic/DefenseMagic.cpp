#include "DefenseMagic.h"

#include "GamePlay/Object/Character/Character.h"

#include "System/Core/Input/Input.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"

#include <imgui.h>

// 初期化
void DefenseMagic::Initialize()
{
	// エフェクトをロードする
}

// 更新処理
void DefenseMagic::Update(Character* owner, float elapsedTime)
{
	if (isActive)
	{
		// 防御魔法を出した位置に展開する
		gaurdPosition = owner->GetPosition();
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

// GUI描画
void DefenseMagic::DrawGUI()
{
	ImGui::Begin(u8"防御魔法");
	ImGui::Text(u8"位置: %.2f, %.2f, %.2f", gaurdPosition.x, gaurdPosition.y, gaurdPosition.z);
}