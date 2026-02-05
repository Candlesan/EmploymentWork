#include"ActionDerive.h"
#include"Mathf.h"
#include"Enemy.h"

//移動
ActionBase::State MoveAction::Run(float elapsedTime)
{
	switch (step)
	{
	case 0:
		//モーション設定(未追加)
		owner->SetAnimationState(Enemy::AnimationState::Walk);
		step++;
		break;
	case 1:
		//目的地設定
		DirectX::XMFLOAT3 position = owner->GetPosition();
		DirectX::XMFLOAT3 targetPosition = owner->GetTargetPosition();
		float vx = targetPosition.x - position.x;
		float vz = targetPosition.z - position.z;
		float distSq = vx * vx + vz * vz;

		//目的地へ移動
		owner->MoveToTarget(elapsedTime, 0.5f);

		//目的地到着
		float radius = owner->GetRadius();
		if (distSq < radius * radius)
		{
			//stepを戻す
			step = 0;
			//成功を返す
			return ActionBase::State::Complete;
		}
		break;
	}
	//実行中を返す
	return ActionBase::State::Run;
}

//停止
ActionBase::State StopAction::Run(float elapsedTime)
{
	//タイマー取得
	float runTimer = owner->GetRunTimer();
	switch (step)
	{
	case 0:
		//実行タイマーを3～5秒に設定
		owner->SetRunTimer(Mathf::RandomRange(3.0f, 5.0f));
		//アニメーションがあればここにセット
		owner->SetAnimationState(Enemy::AnimationState::Idle);
		step++;
		break;
	case 1:
		runTimer -= elapsedTime;
		//タイマー更新
		owner->SetRunTimer(runTimer);

		//待機時間が過ぎた
		if (runTimer <= 0.0f)
		{
			//目的地を設定
			owner->SetRandomTargetPosition();
			//stepをリセット
			step = 0;
			//成功を返す
			return ActionBase::State::Complete;
		}
		break;
	}
	//実行中を返す
	return ActionBase::State::Run;
}