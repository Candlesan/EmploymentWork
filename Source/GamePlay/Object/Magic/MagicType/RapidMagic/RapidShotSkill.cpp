#include "RapidShotSkill.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"

// コンストラクタ
RapidShotSkill::RapidShotSkill()
{
	// 連射魔法を初期化
	rapidMagic.Initialize();
	config.Load(); // Jsonを読み込み

	// エフェクトを設定
	rapidMagic.SetEffect("RapidMagic");
	rapidMagic.SetHitEffect("RapidHit");

	// 連射魔法の設定
	//config.damage		= 0.01f;  // キャラクターに与えるダメージ
	//config.gaurdDamage	= 2.0f;	  // 防御魔法に与えるダメージ
	//config.hitRadius	= 0.25f;  // 当たり判定の大きさ
	//config.speed		= 150.0f; // 弾速
}

void RapidShotSkill::OnHold(const MagicContext& ctx, float elapsedTime)
{
	if (!ctx.canCast || ctx.isCounterActive) return;

	MagicSpawnInfo spawnInfo = ctx.base; // MagicSpawnInfoをまとめてコピー

	// 目標を設定
	spawnInfo.direction = ctx.lockOn->GetAimDirection(ctx.base.position, config.speed);
	spawnInfo.targetPos = ctx.lockOn->IsLocked()
		? ctx.lockOn->GetPredictedPosition(ctx.base.position, config.speed)
		: ctx.defaultTarget;

	// 連射魔法を呼び出す
	rapidMagic.Update(elapsedTime, ctx.magicManager, spawnInfo, config, true);
}

void RapidShotSkill::DrawGUI()
{
}
