#pragma once

class Enemy;

// Às”»’è
class JudgmentBase
{
public:
	JudgmentBase(Enemy* enemy) : owner(enemy) {}
	virtual ~JudgmentBase() {}
	virtual bool Judgment() = 0;
protected:
	Enemy* owner;
};
