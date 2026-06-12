#include "WindBladeMagic.h"
#include "GamePlay/Object/Character/Player/Magic/MagicMove/StraightMagic/StraightMagic.h"
#include "GamePlay/Object/Character/Player/Magic/MagicMove/HommingMagic/HommingMagic.h"


// 球を生成する関数
void WindBladeMagic::Emit(MagicManager* manager, DirectX::XMFLOAT3 playerPos, float playerAngleY, Enemy* enemy, int level)
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
		StraightMagic* magic = new StraightMagic(manager);
		magic->Launch(dir, pos);
	}
	else if (level == 2)
	{
		// 1. 発射方向（プレイヤーの正面方向）
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(playerAngleY);
		dir.y = 0.0f;
		dir.z = cosf(playerAngleY);

		// 2. 基準となる高さ（頭上）
		float baseHeight = playerPos.y + 2.2f;

		// 3. プレイヤーから見た「ローカルな位置（仮の配置）」を決める
		DirectX::XMFLOAT3 localPos[3] = {
			{ -1.2f, -0.3f,  0.0f }, // 左の球
			{  0.0f,  0.3f,  0.0f }, // 中央の球
			{  1.2f, -0.3f,  0.0f }  // 右の球
		};

		// 4. 3回ループして、プレイヤーの向きに合わせて配置する
		for (int i = 0; i < 3; ++i)
		{
			float rotatedX = localPos[i].x * cosf(playerAngleY) + localPos[i].z * sinf(playerAngleY);
			float rotatedZ = -localPos[i].x * sinf(playerAngleY) + localPos[i].z * cosf(playerAngleY);

			// 5. 回転させたズレを、実際のプレイヤーの座標に足し算する
			DirectX::XMFLOAT3 pos;
			pos.x = playerPos.x + rotatedX;
			pos.y = baseHeight + localPos[i].y;
			pos.z = playerPos.z + rotatedZ;

			// 生成して発射
			StraightMagic* magic = new StraightMagic(manager);
			magic->Launch(dir, pos);
		}
	}
	else if (level == 3)
	{
		DirectX::XMFLOAT3 pos;
		pos.x = playerPos.x;
		pos.y = playerPos.y + 1.0f;
		pos.z = playerPos.z;

		//  ターゲット（一番近い敵）を探す処理
		DirectX::XMFLOAT3 target;
		target.x = pos.x + sinf(playerAngleY) * 1000.0f; // 仮のターゲット
		target.y = pos.y;
		target.z = pos.z + cosf(playerAngleY) * 1000.0f;

		float dist = FLT_MAX;
		DirectX::XMVECTOR Postion = DirectX::XMLoadFloat3(&playerPos);
		DirectX::XMVECTOR EPosition = DirectX::XMLoadFloat3(&enemy->GetPosition());
		DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Postion, EPosition);
		float d;
		DirectX::XMStoreFloat(&d, DirectX::XMVector3LengthSq(Vec));
		if (d < dist)
		{
			dist = d;
			target = enemy->GetPosition();
			target.y += enemy->GetHeight() * 0.5f;
		}

		// 5発撃つための角度の準備
		// 足していく間隔は 15度
		float angleStep = DirectX::XMConvertToRadians(15.0f);
		// スタートは同じく -30度
		float currentAngle = playerAngleY - DirectX::XMConvertToRadians(30.0f);

		// 5回ループさせる
		for (int i = 0; i < 5; ++i)
		{
			DirectX::XMFLOAT3 dir;
			dir.x = sinf(currentAngle);
			dir.y = 0.0f;
			dir.z = cosf(currentAngle);

			// 生成して発射！
			HommingMagic* magic = new HommingMagic(manager);
			magic->Launch(dir, pos, target);

			// 撃ち終わったら +15度 足す
			currentAngle += angleStep;
		}
	}
}
