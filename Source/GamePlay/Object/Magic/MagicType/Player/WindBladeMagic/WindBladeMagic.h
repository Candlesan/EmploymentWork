#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"

class WindBladeMagic
{
public:
	// ‹…‚ğ¶¬‚·‚éŠÖ”
	static void Emit(MagicManager* manager, DirectX::XMFLOAT3 playerPos, float playerAngleY, Enemy* enemy, int level);
};

