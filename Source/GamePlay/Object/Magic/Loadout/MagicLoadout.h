#pragma once
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"
#include "GamePlay/Object/Magic/Loadout/MagicSlot.h"

class ShapeRenderer;

class MagicLoadout
{
public:
	// 更新処理
	void Update(const MagicContext& ctx, const MagicCommand& cmd, float elapsedTime);

	// スロットの中身を返す関数
	MagicSlot& GetSlot(int index) { return slots[static_cast<int>(index)]; }

	// GUI描画
	void DrawGUI();

	// デバッグプリミティブ
	void RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer);

	void Clear() { for (auto& slot : slots) slot.Clear(); }

	void SetupFromCatalog(); // 表を読んで、スロットに候補を入れる
private:
	MagicSlot slots[MAGIC_SLOT_COUNT];
};