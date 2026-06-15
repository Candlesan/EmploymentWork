#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"


// 攻撃に遷移できるか判定
bool AttackJudgment::Judgment()
{
    // プレイヤーの距離を取得
    float dist = owner->GetDistanceToPlayer();

    if (dist > Short_Distance)
    {
        return true;
    }
}

// 追跡に遷移できるか判定
bool PursuitJudgment::Judgment()
{
    // プレイヤーの距離を取得
    float dist = owner->GetDistanceToPlayer();

    // Short_Distanceより離れていたら遷移
    if (dist > Short_Distance)
    {
        return true;
    }

    return false;
}

// 徘徊に遷移できるか判定
bool WanderJudgment::Judgment()
{
    return false;
}

bool FloatConditionJudgment::Judgment()
{
    BTConditionType type = static_cast<BTConditionType>(ConditionType);

    switch (type)
    {
    case BTConditionType::Distance:     // プレイヤーとの距離判定
        if (owner->GetAttackCoolTimer() > 0.0f)
        {
            return false; // クールダウン中だから、今は攻撃条件を不成立（false）にする！
        }

    return owner->GetDistanceToPlayer() <= Threshold;
    case BTConditionType::BossHpRate:   // ボスのHPの割合
        return (owner->GetHealth() / owner->GetMaxHealth() * 100.0f) <= Threshold;
    case BTConditionType::timer:        // タイマー
        return owner->GetRunTimer() >= Threshold;
    default:
        return false;
    }
}