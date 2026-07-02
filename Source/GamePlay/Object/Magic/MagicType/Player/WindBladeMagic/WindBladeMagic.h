#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"

class WindBladeMagic
{
public:
	struct WindBladeConfig
	{
		float speed = 15.0f; // 速度
		float damage = 100.0f; // ダメージ
		float waitTimer = 0.5f; // 発射ディレイ
		float lifeTimer = 3.0f; // 寿命
	};

	// 球を生成する関数
	static void Emit(MagicManager* manager, DirectX::XMFLOAT3 playerPos, float playerAngleY, Enemy* enemy, int level, const WindBladeConfig& config);
};

