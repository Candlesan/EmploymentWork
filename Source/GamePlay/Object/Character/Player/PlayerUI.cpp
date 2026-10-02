#include "PlayerUI.h"
#include "Gameplay/Object/Character/Character.h"
#include <algorithm>


// 初期化
void PlayerUI::Initialize(UICanvas& uiCanvas)
{
	hpGauge = uiCanvas.Find("HP");
	mpGauge = uiCanvas.Find("MP");
	guardGauge = uiCanvas.Find("GuardBar");
	heatGauge = uiCanvas.Find("HeatBar");
}

// 更新処理
void PlayerUI::Update(const CharacterStatus& status)
{
	// HP・MPなどの現在の数値から最大値までを0～1に直すラムダ関数
	auto setFill = [](UIElement* e, float current, float max)
	{
		if (!e || max <= 0.0f) return; // UIElementが無いまたは最大値が0以下なら処理をしない
		e->fillAmount = std::clamp(current / max, 0.0f, 1.0f); // 0～1に直す
	};

	setFill(hpGauge, status.currentHp, status.maxHp);
	setFill(mpGauge, status.currentMagic, status.maxMagic);
	setFill(guardGauge, status.currentDefenseDurability, status.maxDefenseDurability);
	setFill(heatGauge, status.currentHeat, status.maxHeat);
}

