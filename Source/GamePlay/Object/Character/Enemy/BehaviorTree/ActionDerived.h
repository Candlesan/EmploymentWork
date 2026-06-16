#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/ActionBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"


// 待機行動
class IdleAction : public ActionBase
{
public:
	IdleAction(Enemy* enemy) : ActionBase(enemy) {}
	ActionBase::State Run(float elapsedTime);
};

// 徘徊行動
class WanderAction : public ActionBase
{
public:
	WanderAction(Enemy* enemy) : ActionBase(enemy) {}
	ActionBase::State Run(float elapsedTime);
private:
	float Short_Distance = 4.0f; // 近距離
	float Middle_Distance = 12.0; // 中距離
	float Long_Distance = 20.0; // 遠距離
	int WalkDirection = 0;
};

// 攻撃行動
class AttackAction : public ActionBase
{
public:
	AttackAction(Enemy* enemy) : ActionBase(enemy) {}
	ActionBase::State Run(float elapsedTime);
};

// 追跡行動
class PursuitAction : public ActionBase
{
public:
	PursuitAction(Enemy* enemy) : ActionBase(enemy){}
	ActionBase::State Run(float elapsedTime);
private:
	float Short_Distance = 4.0f; // 近距離
	float Middle_Distance = 12.0; // 中距離
	float Long_Distance = 20.0; // 遠距離
};
