#include "MagicCatalog.h"

#include "GamePlay/Object/Magic/MagicType/ShotgunMagic/ShotgunSkill.h"
#include "GamePlay/Object/Magic/MagicType/RapidMagic/RapidShotSkill.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/LaserShotSkill.h"
#include "GamePlay/Object/Magic/MagicType/InterceptMagic/InterceptSkill.h"
#include "GamePlay/Object/Magic/MagicType/DefenseMagic/DefenseSkill.h"

// 魔法を作る関数
template<typename T>
static std::unique_ptr<IMagicSkill> Create() { return std::make_unique<T>(); }

// スロットごとの表(順番 = 番号。UIのfocusIndexと揃える)
static const std::vector<MagicEntry> entries[MAGIC_SLOT_COUNT] =
{
    // 関数そのもの(の場所)を渡す
    /* RB */ { { &Create<LaserShotSkill> } },
    /* RT */ { { &Create<ShotgunSkill> }, { &Create<RapidShotSkill> } },
    /* LB */ { { &Create<DefenseSkill> } },
    /* LT */ { { &Create<InterceptSkill> } },
};

// そのスロットに付けられる魔法の一覧(並び順 = 番号 = UIのfocusIndex)
const std::vector<MagicEntry>& MagicCatalog::GetEntries(MagicSlotIndex slot)
{
    return entries[static_cast<int>(slot)];
}

// そのスロットの魔法の数
int MagicCatalog::GetCount(MagicSlotIndex slot)
{
    return static_cast<int>(GetEntries(slot).size());
}
