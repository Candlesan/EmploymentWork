#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"


class LaserShotSkill : public IMagicSkill
{
public:
	LaserShotSkill();
	void OnPress(const MagicContext& ctx) override;
	void Update(const MagicContext& ctx, float elapsedTime) override;
	void DrawGUI() override;
	const char* GetName() const override { return u8"レーザー魔法"; }

private:
	NormalMagic normalMagic; // 技専用のインスタンス
	NormalMagic::NormalMagicConfig config; // 技専用の設定

	float coolDown = 3.0f; // 撃った後のクールダウン
	float coolDownTimer = 0.0f; // 残り時間
};