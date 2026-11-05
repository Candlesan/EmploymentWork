#include "MagicLoadout.h"

// 更新処理
void MagicLoadout::Update(const MagicContext& ctx, const MagicCommand& cmd, float elapsedTime)
{
	for (int i = 0; i < 4; ++i)
	{
		IMagicSkill* skill = slots[i].GetEquipped(); // 装備を取得する
		if (!skill) continue;

		if (cmd.press[i]) skill->OnPress(ctx);
		if (cmd.hold[i]) skill->OnHold(ctx, elapsedTime);
		if (cmd.release[i]) skill->OnRelease(ctx);
		skill->Update(ctx, elapsedTime);
	}
}

// GUI描画
void MagicLoadout::DrawGUI()
{
	for (int i = 0; i < 4; ++i)
	{
		IMagicSkill* skill = slots[i].GetEquipped(); // 装備を取得する
		if (!skill) continue;

		skill->DrawGUI();
	}
}

// デバックプリミティブ描画
void MagicLoadout::RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer)
{
	for (int i = 0; i < 4; ++i)
	{
		IMagicSkill* skill = slots[i].GetEquipped(); // 装備を取得する
		if (!skill) continue;

		skill->RenderDebugPrimitive(ctx, renderer);
	}
}
