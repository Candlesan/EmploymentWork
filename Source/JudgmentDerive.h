#pragma once
#include"JudgmentBase.h"
#include"Enemy.h"

//MoveNode‚ÉˆÚ“®‚Å‚«‚é‚©”»’è
class MoveJudgment : public JudgmentBase
{
public:
	MoveJudgment(Enemy* enemy) :JudgmentBase(enemy) {};
	bool Judgment();
};

//StopNode‚ÉˆÚs‚Å‚«‚é‚©”»’è
//class StopJudgment : public JudgmentBase
//{
//public:
//	StopJudgment(Cattle* cattle) :JudgmentBase(cattle) {};
//	bool Judgment();
//};