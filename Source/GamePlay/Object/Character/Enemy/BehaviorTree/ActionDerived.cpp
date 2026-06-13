#include "ActionDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "System/Core/Mathf.h"

#include <stdlib.h>
#include <string>


// 待機行動
ActionBase::State IdleAction::Run(float elapsedTime)
{
    // 待機アニメーションを再生
    owner->ChangeAnimationState(animName);

    // プレイヤーの方をじっと見つめる
    owner->TurnToPosition(elapsedTime);

    // 常に完了扱いにしておけば、毎フレーム「攻撃できるかな？」「離れたかな？」と再評価できる
    return ActionBase::State::Complete;
}

// 徘徊行動
ActionBase::State WanderAction::Run(float elapsedTime)
{
    return ActionBase::State::Complete;
}

// 攻撃行動
ActionBase::State AttackAction::Run(float elapsedTime)
{
    return ActionBase::State::Complete;
}

// 追跡行動
ActionBase::State PursuitAction::Run(float elapsedTime)
{
	owner->ChangeAnimationState(animName);

    float dist = owner->GetDistanceToPlayer();

    if (dist <= Short_Distance) {
        return ActionBase::State::Complete;
    }

    return ActionBase::State::Run;
}