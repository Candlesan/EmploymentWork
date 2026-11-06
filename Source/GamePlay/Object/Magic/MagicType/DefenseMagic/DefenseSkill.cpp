#include "DefenseSkill.h"
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Magic/MagicType/DefenseMagic/DefenseMagic.h"

// コンストラクタ
DefenseSkill::DefenseSkill()
{
	CustomEffectManager::Instance().LoadEffectConfig("Barreir", "Data/Json/Effect/Barreir.json"); // 防御魔法のエフェクトの読み込み
}

// 長押し
void DefenseSkill::OnHold(const MagicContext& ctx, float elapsedTime)
{
	bool isBreak = ctx.base.owner->GetStatus().isBreak;
	bool canUse = (!isBreak && ctx.canCast);

	// ブレイクしてなくて操作できる状態なら
	if (canUse)
	{
		ctx.base.owner->SetIsGaurd(true);

		// まだ生きているかを確認する
		if (!effectManager.Exists(defenseMagicEffect))
		{
			defenseMagicEffect = effectManager.PlayEffect("Barreir", ctx.base.position, {0, 0, 0}, { 1.5f, 1.5f, 1.5f });
		}
		else
		{
			// 出してる間はプレイヤーに追従させる
			effectManager.SetPosition(defenseMagicEffect, ctx.base.owner->GetDefenceMagic().GetDefensePosition());
		}
	}
}

// 離したとき
void DefenseSkill::OnRelease(const MagicContext& ctx)
{
	ctx.base.owner->SetIsGaurd(false);

	// ボタンを離したら止めて、ポインタも空に戻す
	if (defenseMagicEffect)
	{
		effectManager.Stop(defenseMagicEffect);
		defenseMagicEffect = InvalidEffect;
	}
}

// 更新処理
void DefenseSkill::Update(const MagicContext& ctx, float elapsedTime)
{
	bool isBreak = ctx.base.owner->GetStatus().isBreak;
	if (!isBreak && ctx.canCast) return; // 使える状態なら何もしない

	// 押している途中でブレイクした・操作できなくなったら解く
	ctx.base.owner->SetIsGaurd(false);

	if (defenseMagicEffect)
	{
		effectManager.Stop(defenseMagicEffect);
		defenseMagicEffect = InvalidEffect;
	}
}

// GUI描画
void DefenseSkill::DrawGUI()
{
}