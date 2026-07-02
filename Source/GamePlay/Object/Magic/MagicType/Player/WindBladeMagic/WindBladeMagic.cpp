#include "WindBladeMagic.h"
#include "System/Core/Mathf.h"
#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"


// 球を生成する関数
void WindBladeMagic::Emit(MagicManager* manager, DirectX::XMFLOAT3 playerPos, float playerAngleY, Enemy* enemy, int level, const WindBladeConfig& config)
{
	//レベルによって挙動を変える
	if (level == 1)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(playerAngleY);
		dir.y = 0.0f;
		dir.z = cosf(playerAngleY);
		// 発射位置（仮でプレイヤーの腰当たり）
		DirectX::XMFLOAT3 pos;
		pos.x = playerPos.x;
		pos.y = playerPos.y + 1.0f;// 仮で腰当たりに配置
		pos.z = playerPos.z;

		// 発射
		MagicProjectile* magic = new MagicProjectile(manager, "Data/Effect/WindBleade/WindBlead.efk");
		magic->SetDamage(config.damage);
		magic->SetSpeed(config.speed);
		magic->SetLifeTimer(config.lifeTimer);

		// まっすぐ飛ぶ部品を作って、弾本体にハメ込む！
		magic->SetMoveComponent(std::make_unique<StraightMove>());

		magic->Launch(dir, pos, config.waitTimer);
	}
	//else if (level == 2)
	//{
	//	// 1. 発射方向（プレイヤーの正面方向）
	//	DirectX::XMFLOAT3 dir;
	//	dir.x = sinf(playerAngleY);
	//	dir.y = 0.0f;
	//	dir.z = cosf(playerAngleY);

	//	// 2. 基準となる高さ（頭上）
	//	float baseHeight = playerPos.y + 2.2f;

	//	// 3. プレイヤーから見た「ローカルな位置（仮の配置）」を決める
	//	DirectX::XMFLOAT3 localPos[3] = {
	//		{ -1.2f, -0.3f,  0.0f }, // 左の球
	//		{  0.0f,  0.3f,  0.0f }, // 中央の球
	//		{  1.2f, -0.3f,  0.0f }  // 右の球
	//	};

	//	// 4. 3回ループして、プレイヤーの向きに合わせて配置する
	//	for (int i = 0; i < 3; ++i)
	//	{
	//		float rotatedX = localPos[i].x * cosf(playerAngleY) + localPos[i].z * sinf(playerAngleY);
	//		float rotatedZ = -localPos[i].x * sinf(playerAngleY) + localPos[i].z * cosf(playerAngleY);

	//		// 5. 回転させたズレを、実際のプレイヤーの座標に足し算する
	//		DirectX::XMFLOAT3 pos;
	//		pos.x = playerPos.x + rotatedX;
	//		pos.y = baseHeight + localPos[i].y;
	//		pos.z = playerPos.z + rotatedZ;

	//		// 生成して発射
	//		StraightMagic* magic = new StraightMagic(manager, "Data/Effect/Enemy_Bullet/Enemy_Bullet.efk", "Data/Effect/magic_circle/magic_circle_wind.efk");
	//		magic->SetDamage(80);
	//		magic->Launch(dir, pos, 1.5);
	//	}
	//}
	//else if (level == 3)
	//{
	//	DirectX::XMFLOAT3 pos;
	//	pos.x = playerPos.x + sinf(playerAngleY) * 2.0f;
	//	pos.y = playerPos.y + 1.0f;
	//	pos.z = playerPos.z + 1.0f;

	//	//  ターゲット（一番近い敵）を探す処理
	//	DirectX::XMFLOAT3 target;
	//	target.x = pos.x + sinf(playerAngleY) * 1000.0f; // 仮のターゲット
	//	target.y = pos.y;
	//	target.z = pos.z + cosf(playerAngleY) * 1000.0f;

	//	float dist = FLT_MAX;
	//	DirectX::XMVECTOR Postion = DirectX::XMLoadFloat3(&playerPos);
	//	DirectX::XMVECTOR EPosition = DirectX::XMLoadFloat3(&enemy->GetPosition());
	//	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Postion, EPosition);
	//	float d;
	//	DirectX::XMStoreFloat(&d, DirectX::XMVector3LengthSq(Vec));
	//	if (d < dist)
	//	{
	//		dist = d;
	//		target = enemy->GetPosition();
	//		target.y += enemy->GetHeight() * 0.5f;
	//	}

	//	// 5回ループさせる
	//	for (int i = 0; i < 5; ++i)
	//	{
	//		DirectX::XMFLOAT3 spawnPos = pos;
	//		spawnPos.x += Mathf::RandomRange(-2.0f, 2.0f);
	//		spawnPos.y += Mathf::RandomRange(-0.5f, 1.5f);
	//		spawnPos.z += 0.0;

	//		DirectX::XMVECTOR pVec = DirectX::XMLoadFloat3(&spawnPos);
	//		DirectX::XMVECTOR tVec = DirectX::XMLoadFloat3(&target);
	//		DirectX::XMVECTOR dirVec = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(tVec, pVec));

	//		DirectX::XMFLOAT3 dir;
	//		DirectX::XMStoreFloat3(&dir, dirVec);

	//		float waitTime = 0.5f + (i * 0.2f);

	//		// 生成して発射！
	//		HommingMagic* magic = new HommingMagic(manager, "Data/Effect/Enemy_Bullet/Enemy_Bullet.efk", "Data/Effect/magic_circle/magic_circle_wind.efk");
	//		magic->SetDamage(50);
	//		magic->Launch(dir, spawnPos, target, waitTime);
	//	}
	//}
}