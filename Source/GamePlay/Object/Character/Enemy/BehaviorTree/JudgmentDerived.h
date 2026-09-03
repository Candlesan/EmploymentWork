#pragma once
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/JudgmentBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"



// 徘徊行動
class WanderJudgment : public JudgmentBase
{
public:
	WanderJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	// 判定
	bool Judgment();
};

// Float型の条件判定
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

// 条件なし用
class NoneJudgment : public JudgmentBase
{
public:
	NoneJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	bool Judgment() override;
};

// 複数の条件のJudgment
class AndJudgment : public JudgmentBase
{
public:

	AndJudgment(Enemy* enemy) : JudgmentBase(enemy) {}
	~AndJudgment() override;

	// 外部から条件を1つづつ追加する溜めの関数
	void AddJudgment(JudgmentBase* j);

	// 内部の全ての条件がクリアしているかを確認する
	bool Judgment() override;

private:
	std::vector<JudgmentBase*> judgments; // 条件の配列
};
