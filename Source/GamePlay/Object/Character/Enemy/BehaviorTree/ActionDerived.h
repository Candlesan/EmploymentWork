#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/ActionBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"


// 待機行動
class Idle : public ActionBase
{
public:
	Idle(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 徘徊行動
class Pursuit : public ActionBase
{
public:
	Pursuit(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
};

// 前進
class MoveForward : public ActionBase
{
public:
	MoveForward(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 後退
class MoveBack : public ActionBase
{
public:
	MoveBack(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 右
class MoveRight : public ActionBase
{
public:
	MoveRight(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);

private:
	float strafeDir = 1.0f;
	float timer = 0.0f;
};

// 左
class MoveLeft : public ActionBase
{
public:
	MoveLeft(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);

private:
	float strafeDir = -1.0f;
	float timer = 0.0f;
};

// 通常攻撃魔法
class NormalMagicAttack : public ActionBase
{
public:
	NormalMagicAttack(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
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