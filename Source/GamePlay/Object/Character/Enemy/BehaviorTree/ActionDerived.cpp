#include "ActionDerived.h"
#include "System/Core/Mathf.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "GamePlay/Object/Magic/MagicType/Enemy/BloodSpear.h"

#include <stdlib.h>
#include <string>


// 待機行動
ActionBase::State IdleAction::Run(float elapsedTime)
{
    float runTimer = owner->GetRunTimer();
    switch (step)
    {
    case 0:
        owner->SetRunTimer(Mathf::RandomRange(3.0f, 5.0f));

        // 待機アニメーションを再生
        owner->ChangeAnimationState(animName);
        step++;
        break;
    case 1:
        runTimer -= elapsedTime;
        // タイマー更新
        owner->SetRunTimer(runTimer);

        // プレイヤーの方をじっと見つめる
        owner->TurnToPosition(elapsedTime);

        // 待機時間が過ぎた時
        if (runTimer <= 0.0f)
        {
            step = 0;
            return ActionBase::State::Complete;
        }
        break;
    }
    
    return ActionBase::State::Run;
}

// 徘徊行動
ActionBase::State WanderAction::Run(float elapsedTime)
{
    return ActionBase::State::Complete;
}

// 攻撃行動
ActionBase::State AttackAction::Run(float elapsedTime)
{
    // アニメーションを設定する
    owner->ChangeAnimationState(animName);

    // プレイヤーの方をじっと見つめる
    owner->TurnToPosition(elapsedTime);

    if (owner->IsAnimationFinished())
    {
        // クールダウンを設定する
        owner->SetAttackCoolTimer(1.0f);

        return ActionBase::State::Complete;
    }

    return ActionBase::State::Run;
}

// 追跡行動
ActionBase::State PursuitAction::Run(float elapsedTime)
{
	owner->ChangeAnimationState(animName);

    float dist = owner->GetDistanceToPlayer();

    // パラメーターを設定する
    owner->SetMoveSpeed(5.0f);
    
    // ターゲットに向かって進む
    owner->MoveToTarget(elapsedTime);

    if (dist <= Short_Distance) {
        return ActionBase::State::Complete;
    }

    return ActionBase::State::Run;
}

void BloodSpearAction::OnEnter()
{
    isAttacking = true;
    owner->ChangeAnimationState(animName);

    // 血の槍を４本生成
    // ボスの位置を基準にして、位置をずらして座標を計算する
    DirectX::XMFLOAT3 bossPos = owner->GetPosition();

    for (int i = 0; i < 4; ++i)
    {
        DirectX::XMFLOAT3 spawnPos = bossPos;
        if (i / 2 == 0)
        {
            spawnPos.x = bossPos.x +  10; // 右側
            spawnPos.y = bossPos.y + (Mathf::RandomRange(1.0f, 3.0f));
            spawnPos.z = bossPos.z; // 前方向には動かさない
        }
        else
        {
            spawnPos.x = bossPos.x - 10; // 左側
            spawnPos.y = bossPos.y + (Mathf::RandomRange(1.0f, 3.0f));
            spawnPos.z = bossPos.z; // 前方向には動かさない
        }

        // 生成時間をずらす
        float waitTime = 1.0f + (i * Mathf::RandomRange(0.1f, 0.5f));

        // 槍を生成する
        BloodSpearMagic* spear = new BloodSpearMagic(&owner->GetMagicManager());
        spear->Launch(spawnPos, waitTime);
    }
}

ActionBase::State BloodSpearAction::Run(float elapsedTime)
{
    // まだこのアクションに入ったばかりなら初期化を呼ぶ
    if (!isAttacking) {
        OnEnter();
    }

    if (owner->IsAnimationFinished())
    {
        isAttacking = false;
        owner->SetAttackCoolTimer(Mathf::RandomRange(1.0f, 3.0f));
        return ActionBase::State::Complete;
    }

    return ActionBase::State::Run;
}