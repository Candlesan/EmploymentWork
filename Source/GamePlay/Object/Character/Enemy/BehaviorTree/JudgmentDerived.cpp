#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"


// UŒ‚‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool AttackJudgment::Judgment()
{
    // 1. ƒN[ƒ‹ƒ^ƒCƒ€’†‚È‚çUŒ‚‚µ‚È‚¢
    if (owner->GetAttackCoolTimer() > 0.0f) return false;

    // 2. ‹——£ƒ`ƒFƒbƒN
    float dist = owner->GetDistanceToPlayer();
    if (dist >= 0 && dist < Long_Distance) return true;

    return false;
}

// ’ÇÕ‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool PursuitJudgment::Judgment()
{
    float dist = owner->GetDistanceToPlayer();

    if (dist <= Short_Distance) {
        return false;
    }

    
    if (dist >= Middle_Distance) {
        return true;
    }

    return false;
}

// œpœj‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool WanderJudgment::Judgment()
{
    float dist = owner->GetDistanceToPlayer();

    if (dist < Long_Distance && owner->GetAttackCoolTimer() > 0.0f)
    {
        return true;
    }

    return false;
}