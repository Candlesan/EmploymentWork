#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"


// UŒ‚‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool AttackJudgment::Judgment()
{
    return false;
}

// ’ÇÕ‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool PursuitJudgment::Judgment()
{
    // ƒvƒŒƒCƒ„[‚Ì‹——£‚ðŽæ“¾
    float dist = owner->GetDistanceToPlayer();

    // Short_Distance‚æ‚è—£‚ê‚Ä‚¢‚½‚ç‘JˆÚ
    if (dist > Short_Distance)
    {
        return true;
    }

    return false;
}

// œpœj‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool WanderJudgment::Judgment()
{
    return false;
}

bool FloatConditionJudgment::Judgment()
{
    BTConditionType type = static_cast<BTConditionType>(ConditionType);

    switch (type)
    {
    case BTConditionType::Distance:     // ƒvƒŒƒCƒ„[‚Æ‚Ì‹——£”»’è
        return owner->GetDistanceToPlayer() > Threshold;
    case BTConditionType::BossHpRate:   // ƒ{ƒX‚ÌHP‚ÌŠ„‡
        return (owner->GetHealth() / owner->GetMaxHealth() * 100.0f) <= Threshold;
    case BTConditionType::timer:        // ƒ^ƒCƒ}[
        return owner->GetRunTimer() >= Threshold;
    default:
        return false;
    }
}