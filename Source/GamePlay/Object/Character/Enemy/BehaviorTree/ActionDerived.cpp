#include "ActionDerived.h"
#include "System/Core/Mathf.h"
#include "GamePlay/Object/Character/Player/Player.h"

#include <stdlib.h>
#include <string>


// 待機行動
ActionBase::State IdleAction::Run(float elapsedTime)
{
    float runTimer = owner->GetRunTimer();
    DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();

    // 待機アニメーションを再生
    owner->ChangeAnimationState(stateName);

    // プレイヤーの方をじっと見つめる
    owner->LockAtTarget(elapsedTime, playerPos);

    return ActionBase::State::Complete;
}

// 追跡行動
ActionBase::State PursuitAction::Run(float elapsedTime)
{
    // プレイヤーの位置を取得する
    DirectX::XMFLOAT3 playerPosition = Player::Instance().GetPosition();

    owner->ChangeAnimationState(stateName);
    owner->LockAtTarget(elapsedTime, playerPosition);

    // 進行方向を計算する
    DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&owner->GetPosition());
    DirectX::XMVECTOR PPos = DirectX::XMLoadFloat3(&playerPosition);
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PPos, EPos);
    Vec = DirectX::XMVector3Normalize(Vec);

    DirectX::XMFLOAT3 moveVec;
    DirectX::XMStoreFloat3(&moveVec, Vec);

    // その方向へ進む
    owner->Move(moveVec.x, moveVec.z, owner->GetMoveSpeed());

    //if (owner->GetDistanceToPlayer() <= 5.0f)
    //{
    //    return ActionBase::State::Complete;
    //}

    return ActionBase::State::Run;
}

// 通常攻撃魔法
ActionBase::State NormalMagicAttackAction::Run(float elapsedTime)
{
    const auto playerPos = Player::Instance().GetPosition();
    owner->LockAtTarget(elapsedTime, playerPos);

    // 事前計算で敵からプレイヤーのベクトルを計算しておく
    DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&owner->GetPosition());
    DirectX::XMVECTOR PPos = DirectX::XMLoadFloat3(&playerPos);
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PPos, EPos);
    Vec = DirectX::XMVector3Normalize(Vec);

    DirectX::XMFLOAT3 shotVec;
    DirectX::XMStoreFloat3(&shotVec, Vec);

    switch (phase)
    {
    case Phase::Start:
        owner->ChangeAnimationState(stateName);
        hasFired = false;
        magicConfig.Load();
        phase = Phase::Attacking;
        return State::Run;

    case Phase::Attacking:
        normalMagic.Update(elapsedTime, owner, magicConfig, false);

        if(!hasFired)
        {
            // 魔法を設定する
            MagicSpawnInfo info;
            info.angleY = owner->GetAngle().y;
            info.direction = shotVec;
            info.position = owner->GetPosition();
            info.holdTime = 0.0f;
            info.owner = owner;
            info.target = &Player::Instance();

            normalMagic.Emit(&owner->GetMagicManager(), info, magicConfig);

            hasFired = true;
        }

        if (owner->GetCurrentAnimationSeconds()
            >= owner->GetCurrentAnimationLength())
        {
            owner->SetAttackCoolTimer(2.0f);
            phase = Phase::Start;
            return State::Complete;
        }

        return State::Run;
    }

    return State::Failed;
}