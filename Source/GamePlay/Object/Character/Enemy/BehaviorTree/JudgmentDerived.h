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

// ”»’è
class FloatConditionJudgment : public JudgmentBase
{
public:
	FloatConditionJudgment(Enemy* enemy, int type, float threshold)
		:JudgmentBase(enemy), ConditionType(type), Threshold(threshold){ }

	bool Judgment() override;

private:
	int ConditionType;
	float Threshold;
};