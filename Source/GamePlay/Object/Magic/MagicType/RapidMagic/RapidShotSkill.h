#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"
#include "GamePlay/Object/Magic/MagicType/RapidMagic/RapidMagic.h"


class RapidShotSkill : public IMagicSkill
{
public:
	RapidShotSkill();
	void OnHold(const MagicContext& ctx, float elapsedTime) override;
	void DrawGUI() override;
	void RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer) override;
	const char* GetName() const override { return u8"連射魔法"; }

private:
	RapidMagic rapidMagic; // 技専用のインスタンス
	RapidMagic::RapidMagicConfig config; // 技専用の設定
};