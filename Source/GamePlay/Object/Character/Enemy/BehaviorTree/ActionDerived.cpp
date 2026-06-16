#include "ActionDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

#include <stdlib.h>
#include <string>


// 待機行動
ActionBase::State IdleAction::Run(float elapsedTime)
{
	//owner->ChangeAnimationState(EnemyAnimationState::Idle_1);
	owner->TurnToPosition(elapsedTime); // 待機中もこっちを見ろ！

	// 1フレームごとに完了を返す
	// これにより、BTは毎フレーム「攻撃できるか？」をチェックし直せる
	return ActionBase::State::Complete;
}

// 徘徊行動
ActionBase::State WanderAction::Run(float elapsedTime)
{
	Player& player = Player::Instance();
	float dist = owner->GetDistanceToPlayer();

	// プレイヤーへの方向ベクトル
	DirectX::XMFLOAT3 bossPos = owner->GetPosition();
	DirectX::XMFLOAT3 playerPos = player.GetPosition();
	float vx = playerPos.x - bossPos.x;
	float vz = playerPos.z - bossPos.z;

	switch (step)
	{
	case 0:
		// 0が左/1が右
		WalkDirection = rand() % 2;

		if (WalkDirection == 0)
		{
			owner->ChangeAnimationState("Run_Bwd"); // 左に歩く
		}
		else
		{
			owner->ChangeAnimationState("Run_Right"); // 右に歩く
		}
		owner->SetMoveSpeed(2.0f); // ゆっくり歩く
		step = 1;
		break;

	case 1:
		owner->TurnToPosition(elapsedTime);

		float moveX, moveZ;
		if (WalkDirection == 0)
		{
			// 左方向：ベクトルを反時計回りに90度回転
			moveX = -vz;
			moveZ = vx;
		}
		else
		{
			// 右方向：ベクトルを時計回りに90度回転
			moveX = vz;
			moveZ = -vx;
		}

		// プレイヤーの周りを回るように移動（sideX, sideZ を使用）
		owner->Move(moveX, moveZ, owner->GetMoveSpeed());

		// クールタイムが終わったら完了してBTに判定を戻す
		if (owner->GetAttackCoolTimer() <= 0.0f)
		{
			step = 0;
			return ActionBase::State::Complete;
		}

		//// プレイヤーから離れすぎたり近すぎたりしたら調整のために完了
		//if (dist > Long_Distance || dist < Short_Distance)
		//{
		//	step = 0;
		//	return ActionBase::State::Complete;
		//}
		break;
	}

	return ActionBase::State::Run;
}

// 攻撃行動
ActionBase::State AttackAction::Run(float elapsedTime)
{
	// 実行中を返す
	return ActionBase::State::Run;
}

// 追跡行動
ActionBase::State PursuitAction::Run(float elapsedTime)
{
	// プレイヤーを取得
	Player& player = Player::Instance();

	// 位置情報を取得する
	DirectX::XMFLOAT3 bossPos = owner->GetPosition();
	DirectX::XMFLOAT3 playerPos = player.GetPosition();

	// プレイヤーへの方向ベクトルを計算
	float vx = playerPos.x - bossPos.x;
	float vz = playerPos.z - bossPos.z;
	float dir = sqrtf(vx * vx + vz * vz);

	// プレイヤーとの距離を取得する
	float distance = owner->GetDistanceToPlayer();

	switch (step)
	{
	case 0:
		// 距離が近かったら完了を返す
		if (distance < Short_Distance)
		{
			step = 0;
			owner->SetBaseSpeed(1.0f); // アニメーション再生速度を等倍に戻す
			return ActionBase::State::Complete;
		}

		if (distance > Short_Distance)
		{
			owner->SetMoveSpeed(7.0f); // 移動速度を設定
			owner->SetBaseSpeed(1.2f); // アニメーション再生速度を設定
			owner->ChangeAnimationState("Run_Fwd"); // 小走りに遷移
			step++;
			break;
		}
	case 1:
		owner->TurnToPosition(elapsedTime, playerPos);

		// 移動処理
		owner->Move(vx, vz, owner->GetMoveSpeed());

		// 終了判定
		//攻撃範囲内に入ったら完了
		if (distance < Short_Distance)
		{
			step = 0;
			return ActionBase::State::Complete;
		}
		else if (distance > Long_Distance)
		{
			step = 0;
			return ActionBase::State::Complete;
		}
	}

	// 実行中を返す
	return ActionBase::State::Run;
}