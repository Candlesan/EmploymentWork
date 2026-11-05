#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"
#include "GamePlay/Object/Magic/MagicType/RapidMagic/RapidMagic.h"


class InterceptSkill : public IMagicSkill
{
public:
	InterceptSkill();
	void OnHold(const MagicContext& ctx, float elapsedTime) override;
	void DrawGUI() override;
	void RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer) override;
	const char* GetName() const override { return u8"迎撃魔法"; }

private:
	RapidMagic rapidMagic; // 技専用のインスタンス
	RapidMagic::RapidMagicConfig config; // 技専用の設定

	float interceptRadius = 50.0f;
	DirectX::XMFLOAT3 centerOffset = { 0.0f, 1.0f, 0.0f };
};