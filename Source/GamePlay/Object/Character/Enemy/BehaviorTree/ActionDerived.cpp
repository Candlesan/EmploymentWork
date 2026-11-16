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

    return State::Complete;
}

// 前進
ActionBase::State MoveForward::Run(float elapsedTime)
{
    const DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    owner->ChangeAnimationState(stateName);

    DirectX::XMFLOAT3 dir = owner->GetMoveDirection(playerPos);
    owner->Move(dir.x, dir.z, owner->GetMoveSpeed());

    return State::Run;
}

// 後退
ActionBase::State MoveBack::Run(float elapsedTime)
{
    const DirectX::XMFLOAT3 playerPos = Player::Instance().GetPosition();
    owner->ChangeAnimationState(stateName);

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

// 回避
ActionBase::State Avoid::Run(float elpasedTime)
{
    if (!isStarted)
    {
        // 今の移動方向を回避方向にする
        DirectX::XMFLOAT3 vel = owner->GetVelocity();
        float len = sqrtf(vel.x * vel.x + vel.z * vel.z);

        // 長さがあったら正規化する
        if (len > 0.1f)
        {
            avoidDir = { vel.x / len, 0.0f, vel.z / len };
        }
        else
        {
            // 止まってるフレーズはプレイヤーに対して左右ランダム
            DirectX::XMFLOAT3 toPlayer = owner->GetMoveDirection(Player::Instance().GetPosition());
            float side = (rand() % 2 == 0) ? 1.0f : -1.0f;
            avoidDir = { toPlayer.x * side, 0.0f, -toPlayer.z * side };
        }

        // 回避方向に身体を傾ける
        DirectX::XMFLOAT3 angle = owner->GetAngle();
        angle.y = atan2f(avoidDir.x, avoidDir.z);
        owner->SetAngle(angle);

        owner->ChangeAnimationState(stateName);
        owner->SetIsAvoid(true);
        isStarted = true;
    }

    owner->Move(avoidDir.x, avoidDir.z, owner->GetMoveSpeed());

    // アニメーションが終わったら終了
    if (owner->GetCurrentAnimationSeconds() >= owner->GetCurrentAnimationLength())
    {
        owner->SetIsAvoid(false);
        isStarted = false;
        owner->SetAvoidCoolDownTimer(Mathf::RandomRange(0.0f, 0.5f));
        return State::Complete;
    }
    return State::Run;
}

// 飛行
ActionBase::State Flight::Run(float elapsedTime)
{
    FlightController& flight = owner->GetFlightController();

    if (!isStarted)
    {
        owner->ChangeAnimationState(stateName);
        flight.SetFlightMode(FlightMode::UP);
        isStarted = true;
    }

    // 目標の高さまで上がったらホバー
    if (owner->GetPosition().y >= targetHeight)
    {
        flight.SetFlightMode(FlightMode::Hover);
        isStarted = false;
        return State::Complete;
    }

    return State::Run;
}

// 降下
ActionBase::State Land::Run(float elapsedTime)
{
    FlightController& flight = owner->GetFlightController();

    if (!isStarted)
    {
        owner->ChangeAnimationState(stateName);
        flight.SetFlightMode(FlightMode::Down);
        isStarted = true;
    }

    if (!flight.IsFlying())
    {
        isStarted = false;
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
        owner->StartOverlayAnimation(stateName);
        hasFired = false;
        magicConfig.Load();
        phase = Phase::Attacking;
        return State::Run;

    case Phase::Attacking:
        normalMagic.Update(elapsedTime, owner, magicConfig, false);

        if(!hasFired && owner->GetAnimSequence().GetEvent("Eshot"))
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
            owner->SetAttackCoolTimer(1.0f);

            hasFired = true;
        }

        if (owner->GetOverlayAnimationSeconds()
            >= owner->GetOverlayAnimationLength())
        {
            owner->StopOverlayAnimation();
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

    LockOnSystem& lockOn = owner->GetLockOnSystem();

    switch (phase)
    {
    case Phase::Start:
        owner->StartOverlayAnimation(stateName);
        magicConfig.Load();
        fireTimer = Mathf::RandomRange(1.0f, 5.0f);
        phase = Phase::Attacking;
        return State::Run;

    case Phase::Attacking:
        // 撃つ方向を計算する
        DirectX::XMFLOAT3 myPos = owner->GetPosition();
        DirectX::XMFLOAT3 aimPos = lockOn.GetPredictedPosition(myPos, magicConfig.speed);

        DirectX::XMVECTOR MyPos = DirectX::XMLoadFloat3(&myPos);

        // 予測位置までの距離
        float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(
            DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&aimPos), MyPos)));

        // 角度で誤差を決める（距離に比例させる）
        const float errorAngle = 2.0f; // 最大誤差（度）
        float leadError = distance * std::tanf(DirectX::XMConvertToRadians(errorAngle));

        aimPos.x += Mathf::RandomRange(-leadError, leadError);
        aimPos.y += Mathf::RandomRange(-leadError, leadError);
        aimPos.z += Mathf::RandomRange(-leadError, leadError);

        // 誤差込みの狙い位置から方向を出す
        DirectX::XMVECTOR Vec = DirectX::XMVector3Normalize(
            DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&aimPos), MyPos));

        DirectX::XMFLOAT3 dir;
        DirectX::XMStoreFloat3(&dir, Vec);

        // 魔法を設定する
        MagicSpawnInfo info;
        info.angleY = owner->GetAngle().y;
        info.direction = dir;       // 発射位置オフセットの向き用
        info.targetPos = aimPos;    // 実際の弾道
        info.position = myPos;
        info.holdTime = 0.0f;
        info.owner = owner;
        info.target = &Player::Instance();
        owner->SetIsRapidFire(true);

        rapidMagic.SetEffect("RapidMagic");
        rapidMagic.SetHitEffect("RapidHit");

        rapidMagic.Update(elapsedTime, &owner->GetMagicManager(), info, magicConfig, true);

        fireTimer -= elapsedTime;

        // 射撃時間が終わったまたは魔力が足りなくなったら終了
        if (fireTimer <= 0.0f || owner->GetCurrentMagic() <= magicConfig.magicCost)
        {
            //owner->SetAttackCoolTimer(Mathf::RandomRange(2.0f, 5.0f));
            owner->SetIsRapidFire(false);
            owner->StopOverlayAnimation();
            phase = Phase::Start;
            return State::Complete;
        }

        return State::Run;
    }

    return State::Failed;
}

// 防御魔法
ActionBase::State GuardMagic::Run(float elapsedTime)
{
    // 防御魔法を起動する
    owner->SetIsGaurd(true);

    if (owner->GetIsGaurd())
    {
        // まだ生きているか確認する
        if (!effectManager.Exists(defenseEffect))
        {
            DirectX::XMFLOAT3 angle = {
                DirectX::XMConvertToDegrees(owner->GetAngle().x),
                DirectX::XMConvertToDegrees(owner->GetAngle().y),
                DirectX::XMConvertToDegrees(owner->GetAngle().z)
            };

            defenseEffect = CustomEffectManager::Instance().PlayEffect("Barreir", owner->GetPosition(), angle, { 1.5f, 1.5f, 1.5f });
        }
        else
        {
            // 出してる間はプレイヤーに追従させる
            effectManager.SetPosition(defenseEffect,owner->GetDefenseMagic().GetDefensePosition());
        }
    }
    else
    {
        // フラグが折れていたらエフェクトも止める
        if (defenseEffect)
        {
            effectManager.Stop(defenseEffect);
            defenseEffect = InvalidEffect;
        }
    }

    return State::Run; // 張り続ける（ノードを「途中中断可」にして解除に乗り換えさせる）
}

// 防御解除
ActionBase::State DropGuardMagic::Run(float elapsedTime)
{
    // 防御魔法を解除する
    owner->SetIsGaurd(false);

    // 十分に回復したら終わる
    const auto& s = owner->GetStatus();
    if (s.currentMagic >= s.maxMagic * 0.45f &&
        s.currentDefenseDurability >= s.maxDefenseDurability * 0.6f)
    {
        return State::Complete;
    }
    return State::Run;
}

// 迎撃
ActionBase::State InterceptMagic::Run(float elapsedTime)
{
    if (phase == Phase::Start)
    {
        owner->StartOverlayAnimation(stateName);
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

        // 撃つ前に見た目を迎撃用にする
        rapidMagic.SetEffect("InterceptShot");
        rapidMagic.SetHitEffect("Intercept");

        owner->StopOverlayAnimation();
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
    info.owner = owner;
    info.target = &Player::Instance();
    info.targetPos = targetMagic->GetPosition();
    info.holdTime = 0.0f;

    fireTimer -= elapsedTime;
    if (fireTimer <= 0.0f)
    {
        if (targetMagic->GetMagicKind() == MagicKind::Rapid)
        {
            // 連射で迎撃している間
            owner->SetIsIntercept(true);
            owner->SetIsRapidFire(true);

            rapidFireConfig.speed = 40.0f;

            rapidMagic.Emit(&owner->GetMagicManager(), info, rapidFireConfig);
            fireTimer = rapidFireConfig.rapidInterval;
        }

        // 撃ち落とす
        owner->RecoverMagic(targetMagic->GetMagicmagicRefund());
        CustomEffectManager::Instance().PlayBurst("Intercept", targetMagic->GetPosition(), { 0, 0, 0 }, { 1.0f, 1.0f, 1.0f });
        targetMagic->OnTerminate();
    }

    return State::Run;
}