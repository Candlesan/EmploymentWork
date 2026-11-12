#pragma once
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"

#include <memory>
#include <vector>

// 前方宣言
class IMagicSkill; 

// 表の１行：魔法を１つ作る関数
struct MagicEntry
{
	// 関数ポインタ
	std::unique_ptr<IMagicSkill>(*create)();
};

namespace MagicCatalog
{
	// そのスロットに付けられる魔法の一覧(並び順 = 番号 = UIのfocusIndex)
	const std::vector<MagicEntry>& GetEntries(MagicSlotIndex slot);

	// そのスロットの魔法の数
	int GetCount(MagicSlotIndex slot);
}