#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/JudgmentBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"


// BattleNode‚É‘JˆÚ‚Å‚«‚é‚©”»’è
class BattleJudgment : public JudgmentBase
{
public:
	BattleJudgment(Enemy* enemy) :JudgmentBase(enemy) {};
	// ”»’è
	bool Judgment();
};

// œpœjs“®
class WanderJudgment : public JudgmentBase
{
public:
	WanderJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// ”»’è
	bool Judgment();
};

// UŒ‚s“®
class AttackJudgment : public JudgmentBase
{
public:
	AttackJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// ”»’è
	bool Judgment();
};

// ’ÇÕs“®
class PursuitJudgment : public JudgmentBase
{
public:
	PursuitJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// ”»’è
	bool Judgment();
private:
	float Short_Distance = 5.0f; // ‹ß‹——£
	float Middle_Distance = 10.0f; // ’†‹——£
	float Long_Distance = 15.0; // ‰“‹——£
};

// ‘Ò‹@s“®
class IdleJudgment : public JudgmentBase
{
public:
	IdleJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// ”»’è
	bool Judgment();
};