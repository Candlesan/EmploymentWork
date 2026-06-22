#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

#include <cmath>


// 攻撃に遷移できるか判定
bool AttackJudgment::Judgment()
{
    // プレイヤーの距離を取得
    float dist = owner->GetDistanceToPlayer();

    // Short_Distanceより離れているかつクールダウンが無いときに遷移
    if (dist <= Short_Distance && owner->GetAttackCoolTimer() <= 0.0f)
    {
        return true;
    }

    return false;
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

// Float型の条件判定
bool FloatConditionJudgment::Judgment()
{
    BTConditionType conditionType = static_cast<BTConditionType>(ConditionType);
    float currentValue = 0.0f;

    // 条件なしの場合はThreshold・CompareTypeを見ずに即true
    if (conditionType == BTConditionType::None)
    {
        return true;
    }

    switch (conditionType)
    {
    case BTConditionType::Distance:     // プレイヤーとの距離判定
        currentValue = owner->GetDistanceToPlayer();
        break;
    case BTConditionType::BossHpRate:   // ボスのHPの割合
        return (owner->GetHealth() / owner->GetMaxHealth() * 100.0f) <= Threshold;
        break;
    case BTConditionType::timer:        // タイマー
        currentValue = owner->GetRunTimer();
        break;
    case BTConditionType::AttackCoolDown:        // タイマー
        currentValue = owner->GetAttackCoolTimer();
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

// 条件なし用
bool NoneJudgment::Judgment()
{
    return true;
}

AndJudgment::~AndJudgment()
{
    for (JudgmentBase* j : judgments)
    {
        delete j;
    }

    judgments.clear();
}

// 外部から条件を1つづつ追加する溜めの関数
void AndJudgment::AddJudgment(JudgmentBase* j)
{
    if (j != nullptr) judgments.push_back(j);
}

// 内部の全ての条件がクリアしているかを確認する
bool AndJudgment::Judgment()
{
    // 条件が１つもなかったらTrueを返す
    if (judgments.empty()) return true;

    for (auto* j : judgments)
    {
        // １つでもfalseだったらfalseを返す
        if (!j->Judgment()) return false;
    }

    return true;
}
