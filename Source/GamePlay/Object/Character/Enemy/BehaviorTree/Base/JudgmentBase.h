#pragma once

class Enemy;

// Às”»’è
class JudgmentBase
{
public:
	JudgmentBase(Enemy* enemy) : owner(enemy) {}
	virtual bool Judgment() = 0;
protected:
	Enemy* owner;
};
