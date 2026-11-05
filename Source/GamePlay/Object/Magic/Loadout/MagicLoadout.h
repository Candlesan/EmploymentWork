#pragma once
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"
#include "GamePlay/Object/Magic/Loadout/MagicSlot.h"


class MagicLoadout
{
public:
	// 更新処理
	void Update(const MagicContext& ctx, const MagicCommand& cmd, float elapsedTime);

	// スロットの中身を消す関数
	MagicSlot& GetSlot(int index) { return slots[static_cast<int>(index)]; }

	void DrawGUI();
private:
	MagicSlot slots[MAGIC_SLOT_COUNT];
};