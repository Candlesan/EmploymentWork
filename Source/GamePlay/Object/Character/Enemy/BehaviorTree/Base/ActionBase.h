#pragma once
#include <string>

class Enemy;

// 行動処理基底クラス
class ActionBase
{
public:
	ActionBase(Enemy* enemy, std::string stateName = "")
		:owner(enemy), stateName(stateName) {}

	virtual ~ActionBase() = default;

	// 実行処理
	enum class State
	{
		Run,		// 実行中
		Failed,		// 実行失敗
		Complete,	// 実行成功
	};

	// 実行処理(純粋仮想関数)
	virtual ActionBase::State Run(float elapsedTime) = 0;

	// 途中中断したときのリセット関数
	virtual void Abort() { step = 0; }
protected:
	Enemy* owner;
	int step = 0;
	std::string stateName;
};
