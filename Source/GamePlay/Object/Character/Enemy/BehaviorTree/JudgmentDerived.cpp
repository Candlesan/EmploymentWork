#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

#include <cmath>


// Float型の条件判定
bool FloatConditionJudgment::Judgment()
{
    BTConditionType conditionType = static_cast<BTConditionType>(ConditionType);
    float currentValue = 0.0f;

    // プレイヤーを取得する
    Player& player = Player::Instance();

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
        currentValue = owner->GetHealth() / owner->GetMaxHealth() * 100.0f <= Threshold;
        break;
    case BTConditionType::timer:        // タイマー
        currentValue = owner->GetRunTimer();
        break;
    case BTConditionType::AttackCoolDown:        // タイマー
        currentValue = owner->GetAttackCoolTimer();
        break;
    case BTConditionType::EnemyCurrentMagic: // 敵の魔力残量
        currentValue = owner->GetCurrentMagic();
        break;
    case BTConditionType::EnemyCurrentHeat: // 敵の現在のヒート値の上昇量
        currentValue = owner->GetCurrentHeat();
        break;
    case BTConditionType::EnemyCurrentDefense: // 敵の現在の防御魔法の耐久値
        currentValue = owner->GetCurrentDefense();
        break;
    case BTConditionType::PlayerCurrentMagic: // プレイヤーの現在の魔力残量
        currentValue = player.GetCurrentMagic();
        break;
    case BTConditionType::PlayerCurrentHeat: // プレイヤーの現在のヒート値
        currentValue = player.GetCurrentHeat();
        break;
    case BTConditionType::PlayerCurrentDefense: // プレイヤーの魔力残量
        currentValue = player.GetCurrentDefense();
        break;
    case BTConditionType::IsPlayerDefending: // プレイヤーが防御魔法を展開中か (1.0=Yes, 0.0=No)
        currentValue = player.GetIsGaurd() ? 1.0f : 0.0f;
        break;
    case BTConditionType::IncomingMagicCount: // 自身に向かって飛んできている魔法の数 (回避・迎撃判断用)
    {
        // プレイヤーからの魔法を受け取る
        MagicManager& magicManager = Player::Instance().GetMagicManager();
        std::vector<MagicBase*> magic = magicManager.FindNearProjectiles(owner, owner->GetPosition(), owner->GetIntercepRudius());

        int count = 0;
        DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&owner->GetPosition());

        // 内積をとって正か負かで判定をとる
        for (auto& p : magic)
        {
            // 進行ベクトルを計算する
            DirectX::XMVECTOR magicPos = DirectX::XMLoadFloat3(&p->GetPosition());
            DirectX::XMVECTOR moveDir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(magicPos, EPos));

            // 弾の進行方向を取得する
            DirectX::XMVECTOR magicDir = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&p->GetDirection()));

            // 内積をとる
            float Dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(moveDir, magicDir));
            if (Dot < 0.0f)
            {
                count++; // 正だったらカウントを増やす
            }
        }

        currentValue = static_cast<float>(count);
        break;
    }
    case BTConditionType::IsBurst: // バースト中か
        currentValue = owner->GetStatus().isBuring ? 1.0f : 0.0f;
        break;

    case BTConditionType::IsBreak: // バリアブレイク中か
        currentValue = owner->GetStatus().isBreak ? 1.0f : 0.0f;
        break;

    case BTConditionType::Random: // 確率
        currentValue = owner->GetRandomValue();
        break;
    case BTConditionType::IsFlying: // 飛行中か
        currentValue = owner->GetFlightController().IsFlying() ? 1.0f : 0.0f;
        break;
    case BTConditionType::PlayerIsFlying: // プレイヤーが飛行中か
        currentValue = player.GetFlightController().IsFlying() ? 1.0f : 0.0f;
        break;
    case BTConditionType::AvoidCoolDownTimer: // プレイヤーが飛行中か
        currentValue = owner->GetAvoidCoolDownTimer();
        break;
    case BTConditionType::IsDead: // プレイヤーが飛行中か
        currentValue = owner->IsDead() ? 1.0f : 0.0f;
        break;
    }

    // BTCompareTypeに応じてcurrentValueとThresholdを比較する
    BTCompareType compareType = static_cast<BTCompareType>(CompareType);
    switch (compareType)
    {
    case BTCompareType::Greater:      return currentValue > Threshold;
    case BTCompareType::Less:         return currentValue < Threshold;
    case BTCompareType::GreaterEqual: return currentValue >= Threshold;
    case BTCompareType::LessEqual:    return currentValue <= Threshold;
    case BTCompareType::Equal:        return fabsf(currentValue - Threshold) <= 0.0001f; // floatは誤差があるから「==」じゃなくて差の絶対値で判定する
    case BTCompareType::NotEqual:     return fabsf(currentValue - Threshold) > 0.0001f; // 上のと同じで誤差を含めて判定した不一致の判定
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