#include "ActionDerived.h"
#include "System/Core/Mathf.h"
#include "GamePlay/Object/Character/Player/Player.h"

#include <stdlib.h>
#include <string>


// 待機行動
ActionBase::State Idle::Run(float elapsedTime)
{
    float runTimer = owner->GetRunTimer();
    DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();

    // 待機アニメーションを再生
    owner->ChangeAnimationState(stateName);

    // プレイヤーの方をじっと見つめる
    owner->LockAtTarget(elapsedTime, playerPos);

    return State::Complete;
}

// 前進
ActionBase::State MoveForward::Run(float elapsedTime)
{
    const DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    owner->ChangeAnimationState(stateName);
    owner->LockAtTarget(elapsedTime, playerPos);

    DirectX::XMFLOAT3 dir = owner->GetMoveDirection(playerPos);
    owner->Move(dir.x, dir.z, owner->GetMoveSpeed());

    return State::Run;
}

// 後退
ActionBase::State MoveBack::Run(float elapsedTime)
{
    const DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    owner->ChangeAnimationState(stateName);
    owner->LockAtTarget(elapsedTime, playerPos);

    DirectX::XMFLOAT3 dir = owner->GetMoveDirection(playerPos);
    owner->Move(-dir.x, -dir.z, owner->GetMoveSpeed());

    return State::Run;
}

// 右
ActionBase::State MoveRight::Run(float elapsedTime)
{
    if (timer <= 0.0f) timer = owner->GetRunTimer();

    const DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    owner->ChangeAnimationState(stateName);
    owner->LockAtTarget(elapsedTime, playerPos);

    DirectX::XMVECTOR Dir = DirectX::XMLoadFloat3(&owner->GetMoveDirection(playerPos));
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR Right = DirectX::XMVector3Cross(Up, Dir);

    DirectX::XMFLOAT3 right;
    DirectX::XMStoreFloat3(&right, Right);

    owner->Move(right.x * strafeDir, right.z * strafeDir, owner->GetMoveSpeed());

    owner->SetRunTimer(Mathf::RandomRange(1.0f, 3.0f));

    timer -= elapsedTime;
    if (timer <= 0.0f)
    {
        return State::Complete;
    }

    return State::Run;
}

// 左
ActionBase::State MoveLeft::Run(float elapsedTime)
{
    if (timer <= 0.0f) timer = owner->GetRunTimer();

    const DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    owner->ChangeAnimationState(stateName);
    owner->LockAtTarget(elapsedTime, playerPos);

    DirectX::XMVECTOR Dir = DirectX::XMLoadFloat3(&owner->GetMoveDirection(playerPos));
    DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
    DirectX::XMVECTOR Left = DirectX::XMVector3Cross(Up, Dir);

    DirectX::XMFLOAT3 left;
    DirectX::XMStoreFloat3(&left, Left);

    owner->Move(left.x * strafeDir, left.z * strafeDir, owner->GetMoveSpeed());

    owner->SetRunTimer(Mathf::RandomRange(1.0f, 3.0f));

    timer -= elapsedTime;
    if (timer <= 0.0f)
    {
        return State::Complete;
    }

    return State::Run;
}

// 通常攻撃魔法
ActionBase::State NormalMagicAttack::Run(float elapsedTime)
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
            owner->SetAttackCoolTimer(Mathf::RandomRange(0.0f, 5.0f));
            phase = Phase::Start;
            return State::Complete;
        }

        return State::Run;
    }

    return State::Failed;
}

// 連射魔法
ActionBase::State RapidFireMagic::Run(float elapsedTime)
{
    const auto playerPos = Player::Instance().GetPosition();
    owner->LockAtTarget(elapsedTime, playerPos);

    switch (phase)
    {
    case Phase::Start:
        owner->ChangeAnimationState(stateName);
        magicConfig.Load();
        fireTimer = Mathf::RandomRange(1.0f, 5.0f);
        phase = Phase::Attacking;
        return State::Run;

    case Phase::Attacking:
        DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&owner->GetPosition());
        DirectX::XMVECTOR PPos = DirectX::XMLoadFloat3(&playerPos);
        DirectX::XMVECTOR Vec = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(PPos, EPos));

        // 魔法を設定する
        MagicSpawnInfo info;
        info.angleY = owner->GetAngle().y;
        DirectX::XMStoreFloat3(&info.direction, Vec);
        info.position = owner->GetPosition();
        info.holdTime = 0.0f;
        info.owner = owner;
        info.target = &Player::Instance();

        owner->SetIsRapidFire(true);

        rapidMagic.Update(elapsedTime, &owner->GetMagicManager(), info, magicConfig, true);

        fireTimer -= elapsedTime;
        if (fireTimer <= 0.0f)
        {
            owner->SetAttackCoolTimer(Mathf::RandomRange(2.0f, 5.0f));
            owner->SetIsRapidFire(false);
            phase = Phase::Start;
            return State::Complete;
        }

        return State::Run;
    }

    return State::Failed;
}

// 迎撃
ActionBase::State InterceptMagic::Run(float elapsedTime)
{
    if (phase == Phase::Start)
    {
        owner->ChangeAnimationState(stateName);
        normalConfig.Load();
        rapidFireConfig.Load();
        fireTimer = 0.0f;
        phase = Phase::Attacking;
        return State::Run;
    }

    // 迎撃の中心位置
    DirectX::XMFLOAT3 EPos = owner->GetPosition();
    DirectX::XMFLOAT3 centerPos = { EPos.x, EPos.y + 1.0f, EPos.z };

    // プレイヤーの魔法から一番近いものを取る
    MagicManager& playerManager = Player::Instance().GetMagicManager();
    std::vector<MagicBase*> targets = playerManager.FindNearProjectiles(owner, centerPos, owner->GetIntercepRudius());

    // 撃ち落とす魔法がなくなったら終了
    if (targets.empty())
    {
        phase = Phase::Start;
        owner->SetIsIntercept(false);
        owner->SetIsRapidFire(false);
        return State::Complete;
    }

    MagicBase* targetMagic = targets[0]; // ソート済みの１番近い魔法

    owner->LockAtTarget(elapsedTime, Player::Instance().GetPosition());

    // 迎撃をする方向
    DirectX::XMVECTOR magicPos = DirectX::XMLoadFloat3(&targetMagic->GetPosition());
    DirectX::XMVECTOR myPos = DirectX::XMLoadFloat3(&centerPos);
    DirectX::XMVECTOR Dir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(magicPos, myPos));

    MagicSpawnInfo info;
    info.position = EPos;
    DirectX::XMStoreFloat3(&info.direction, Dir);
    info.angleY = owner->GetAngle().y;
    info.target = &Player::Instance();
    info.holdTime = 0.0f;

    fireTimer -= elapsedTime;
    if (fireTimer <= 0.0f)
    {
        if (targetMagic->GetMagicKind() == MagicKind::Rapid)
        {
            // 連射で迎撃している間
            owner->SetIsIntercept(true);
            owner->SetIsRapidFire(true);

            rapidMagic.Emit(&owner->GetMagicManager(), info, rapidFireConfig);
            fireTimer = rapidFireConfig.rapidInterval;
        }

        // 撃ち落とす
        owner->RecoverMagic(targetMagic->GetMagicmagicRefund());
        targetMagic->OnTerminate();
    }

    return State::Run;
}