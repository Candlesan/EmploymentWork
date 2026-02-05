#pragma once
#include"ActionBase.h"
#include"Enemy.h"

//ˆÚ“®
class MoveAction : public ActionBase
{
public:
	MoveAction(Enemy* enemy) : ActionBase(enemy) {}
	ActionBase::State Run(float elapsedTiem);
};

//’âŽ~
class StopAction : public ActionBase
{
public:
	StopAction(Enemy* enemy) : ActionBase(enemy) {}
	ActionBase::State Run(float elapsedTime);
};