#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/JudgmentBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"


// UŒ‚s“®
class AttackJudgment : public JudgmentBase
{
public:
	AttackJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// ”»’è
	bool Judgment();
private:
	float Short_Distance = 4.0f; // ‹ß‹——£
	float Middle_Distance = 12.0; // ’†‹——£
	float Long_Distance = 20.0; // ‰“‹——£
};

// ’ÇÕs“®
class PursuitJudgment : public JudgmentBase
{
public:
	PursuitJudgment(Enemy* enemy, float threshold) : JudgmentBase(enemy), Short_Distance(threshold) {}
	// ”»’è
	bool Judgment();
private:
	float Short_Distance;  // ‹ß‹——£
	float Middle_Distance = 12.0; // ’†‹——£
	float Long_Distance = 20.0; // ‰“‹——£
};

// œpœjs“®
class WanderJudgment : public JudgmentBase
{
public:
	WanderJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// ”»’è
	bool Judgment();
private:
	float Short_Distance = 4.0f; // ‹ß‹——£
	float Middle_Distance = 12.0; // ’†‹——£
	float Long_Distance = 20.0; // ‰“‹——£
};

// FloatŒ^‚ÌğŒ”»’è
class FloatConditionJudgment : public JudgmentBase
{
public:
	FloatConditionJudgment(Enemy* enemy, int type, float threshold, int compareType)
		:JudgmentBase(enemy), ConditionType(type), Threshold(threshold), CompareType(compareType){ }

	bool Judgment() override;

private:
	int ConditionType;
	int CompareType;
	float Threshold;
};

// ğŒ‚È‚µ—p
class NoneJudgment : public JudgmentBase
{
public:
	NoneJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	bool Judgment() override;
};
