#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

#include <cmath>


// UŒ‚‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool AttackJudgment::Judgment()
{
    return false;
}

// ’ÇÕ‚É‘JˆÚ‚Å‚«‚é‚©”»’è
bool PursuitJudgment::Judgment()
{
    // ƒvƒŒƒCƒ„[‚Ì‹——£‚ğæ“¾
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

// FloatŒ^‚ÌğŒ”»’è
bool FloatConditionJudgment::Judgment()
{
    BTConditionType conditionType = static_cast<BTConditionType>(ConditionType);
    float currentValue = 0.0f;

    // ğŒ‚È‚µ‚Ìê‡‚ÍThresholdECompareType‚ğŒ©‚¸‚É‘¦true
    if (conditionType == BTConditionType::None)
    {
        return true;
    }

    switch (conditionType)
    {
    case BTConditionType::Distance:     // ƒvƒŒƒCƒ„[‚Æ‚Ì‹——£”»’è
        currentValue = owner->GetDistanceToPlayer();
        break;
    case BTConditionType::BossHpRate:   // ƒ{ƒX‚ÌHP‚ÌŠ„‡
        return (owner->GetHealth() / owner->GetMaxHealth() * 100.0f) <= Threshold;
        break;
    case BTConditionType::timer:        // ƒ^ƒCƒ}[
        currentValue = owner->GetRunTimer();
        break;
    }

    BTCompareType compareType = static_cast<BTCompareType>(CompareType);
    switch (compareType)
    {
    case BTCompareType::Greater:      return currentValue > Threshold;
    case BTCompareType::Less:         return currentValue < Threshold;
    case BTCompareType::GreaterEqual: return currentValue >= Threshold;
    case BTCompareType::LessEqual:    return currentValue <= Threshold;
    case BTCompareType::Equal:        return fabsf(currentValue == Threshold) <= 0.0001f;
    case BTCompareType::NotEqual:     return fabsf(currentValue != Threshold) >= 0.0001f;
    default: return false;
    }
}

// ğŒ‚È‚µ—p
bool NoneJudgment::Judgment()
{
    return true;
}
