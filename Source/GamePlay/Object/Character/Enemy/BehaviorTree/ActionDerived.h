#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/ActionBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"


// ë“ã@çsìÆ
class IdleAction : public ActionBase
{
public:
	IdleAction(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// úpújçsìÆ
class PursuitAction : public ActionBase
{
public:
	PursuitAction(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
};

// çUåÇçsìÆ
class AttackAction : public ActionBase
{
public:
	AttackAction(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);

private:
	enum class Phase
	{
		Start,
		Attacking
	};

	Phase phase = Phase::Start;
};