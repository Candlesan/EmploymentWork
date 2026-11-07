#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"
#include "GamePlay/Object/Magic/MagicType/ShotgunMagic/ShotgunMagic.h"


class ShotgunSkill : public IMagicSkill
{
public:
	ShotgunSkill();
	void OnHold(const MagicContext& ctx, float elapsedTime) override;
	void Update(const MagicContext& ctx, float elapsedTime) override;
	void DrawGUI() override;
	const char* GetName() const override { return u8"ショットガン"; }

private:
	ShotgunMagic shotgunMagic; // 技専用のインスタンス
	ShotgunMagic::ShotgunMagicConfig config; // 技専用の設定
	float coolDownTimer = 0.0f;               // 残りのクールダウン
};