#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"


class DefenseSkill : public IMagicSkill
{
public:
	DefenseSkill();
	void OnHold(const MagicContext& ctx, float elapsedTime) override;
	void OnRelease(const MagicContext& ctx) override;
	void Update(const MagicContext& ctx, float elapsedTime) override;
	void DrawGUI() override;
	void OnUnequip(const MagicContext& ctx) override { OnRelease(ctx); }
	const char* GetName() const override { return u8"防御魔法"; }

private:
	EffectHandle defenseMagicEffect = InvalidEffect; // 防御魔法のエフェクト
	CustomEffectManager& effectManager = CustomEffectManager::Instance();
};