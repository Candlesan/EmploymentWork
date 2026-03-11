#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

// í“¬‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool BattleJudgment::Judgment()
{
	float dist = owner->GetDistanceToPlayer();
	return false;
}

// UŒ‚‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool AttackJudgment::Judgment()
{
	return false;
}

// œpœj‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool WanderJudgment::Judgment()
{
	return false;
}

// ’ÇÕ‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool PursuitJudgment::Judgment()
{
    float dist = owner->GetDistanceToPlayer();

    if (dist <= 2.5f) {
        return false;
    }

    // ‰“‹——£ƒ‰ƒCƒ“‚æ‚èŠO‘¤‚à’ÇÕ‚µ‚È‚¢
    if (dist >= Long_Distance) {
        return false;
    }

    return true;
}

// ‘Ò‹@‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool IdleJudgment::Judgment()
{
	return true;
}
