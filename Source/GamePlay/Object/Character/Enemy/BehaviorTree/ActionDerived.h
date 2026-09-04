#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/ActionBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"


// 待機行動
class IdleAction : public ActionBase
{
public:
	IdleAction(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 徘徊行動
class PursuitAction : public ActionBase
{
public:
	PursuitAction(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
};

// 通常攻撃魔法
class NormalMagicAttackAction : public ActionBase
{
public:
	NormalMagicAttackAction(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);

private:
	enum class Phase
	{
		Start,
		Attacking
	};

	Phase phase = Phase::Start;
	NormalMagic normalMagic;
	NormalMagic::NormalMagicConfig magicConfig;
	bool hasFired = false; // 既に魔法を発射したかのフラグ
};