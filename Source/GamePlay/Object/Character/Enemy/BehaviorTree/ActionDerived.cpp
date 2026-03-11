#include "ActionDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

// 待機行動
ActionBase::State IdleAction::Run(float elapsedTime)
{
	// 実行タイマーを取得
	float runTimer = owner->GetRunTimer();

	switch (step)
	{
	case 0:
		owner->SetRunTimer(Mathf::RandomRange(3.0f, 5.0f));
		owner->ChangeAnimationState(EnemyAnimationState::Idle);
		step++;
		break;
	case 1:
		runTimer -= elapsedTime;

		// タイマー更新
		owner->SetRunTimer(runTimer);

		// 待機時間が過ぎた時
		if (runTimer <= 0.0f)
		{
			step = 0;
			return ActionBase::State::Complete;
		}
	}
	
	// 実行中を返す
	return ActionBase::State::Run;
}

// 徘徊行動
ActionBase::State WanderAction::Run(float elapsedTime)
{
	// 実行中を返す
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
		if (distance < 2.0f)
		{
			step = 0;
			owner->SetBaseSpeed(1.0f); // アニメーション再生速度を等倍に戻す
			return ActionBase::State::Complete;
		}

		if (distance > Middle_Distance)
		{
			owner->SetMoveSpeed(7.0f); // 移動速度を設定
			owner->SetBaseSpeed(1.2f); // アニメーション再生速度を設定
			owner->ChangeAnimationState(EnemyAnimationState::Jog_F); // 小走りに遷移
			step++;
			break;
		}
		else
		{
			owner->SetMoveSpeed(5.0f); // 移動速度を設定
			owner->ChangeAnimationState(EnemyAnimationState::Walk_F); // 歩きに遷移
			step++;
			break;
		}
	case 1:
		// 方向ベクトルを正規化
		if (distance > 0.001f)
		{
			vx /= dir;
			vz /= dir;
		}

		// 旋回処理
		owner->Turn(elapsedTime, vx, vz, owner->GetTurnSpeed());

		// 移動処理
		owner->Move(vx, vz, owner->GetMoveSpeed());

		// 終了判定
		//攻撃範囲内に入ったら完了
		if (distance < 2.5)
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