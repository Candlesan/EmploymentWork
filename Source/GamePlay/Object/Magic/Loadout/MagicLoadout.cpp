#include "MagicLoadout.h"
#include "GamePlay/Session/GameSession.h"
#include "GamePlay/Object/Magic/Loadout/MagicCatalog.h"

#include <imgui.h>

// 更新処理
void MagicLoadout::Update(const MagicContext& ctx, const MagicCommand& cmd, float elapsedTime)
{
	for (int i = 0; i < 4; ++i)
	{
		slots[i].ApplyPending(ctx); // 予約されてたら、ここで入れ替える

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
	static const char* slotNames[MAGIC_SLOT_COUNT] = { "RB", "RT", "LB", "LT" };

	for (int i = 0; i < MAGIC_SLOT_COUNT; ++i)
	{
		MagicSlot& slot = slots[i];
		IMagicSkill* equipped = slot.GetEquipped();

		if (ImGui::BeginCombo(slotNames[i], equipped ? equipped->GetName() : "-"))
		{
			for (int c = 0; c < slot.GetCandidateCount(); ++c)
			{
				bool selected = (c == slot.GetEquippedIndex());
				if (ImGui::Selectable(slot.GetCandidate(c)->GetName(), selected))
				{
					GameSession::Instance().equippedIndex[i] = c;
					slot.SetEquipped(c);
				}
			}
			ImGui::EndCombo();
		}
	}

	for (int i = 0; i < MAGIC_SLOT_COUNT; ++i)
	{
		IMagicSkill* skill = slots[i].GetEquipped(); // 装備を取得する
		if (!skill) continue;

		skill->DrawGUI();
	}
}

// デバックプリミティブ描画
void MagicLoadout::RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer)
{
	for (int i = 0; i < MAGIC_SLOT_COUNT; ++i)
	{
		IMagicSkill* skill = slots[i].GetEquipped(); // 装備を取得する
		if (!skill) continue;

		skill->RenderDebugPrimitive(ctx, renderer);
	}
}

// 表を読んで、スロットに候補を入れる
void MagicLoadout::SetupFromCatalog()
{
	// スロットの中身を削除する
	Clear();

	for (int i = 0; i < MAGIC_SLOT_COUNT; ++i)
	{
		for (const MagicEntry& entry : MagicCatalog::GetEntries(static_cast<MagicSlotIndex>(i)))
		{
			slots[i].AddCandidate(entry.create());
		}
	}
}
