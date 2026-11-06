#include "LaserShotSkill.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"
#include "GamePlay/Object/CameraShake/CameraShake.h"


// コンストラクタ
LaserShotSkill::LaserShotSkill()
{
	normalMagic.Initialize();
	config.Load(); // 通常魔法の設定の読み込み

	config.chargeTimeThreshold = 0.0f; // 溜め時間なし → すぐフルチャージになる
	config.chargeHoldMagicCost = 0.0f; // 溜まってる間の魔力消費なし
}

// 押した瞬間
void LaserShotSkill::OnPress(const MagicContext& ctx)
{
	if (!ctx.canCast) return;
	if (coolDownTimer > 0.0f) return;

	// 発射位置
	DirectX::XMFLOAT3 muzzlePos = {
		ctx.base.position.x + config.spawnOffset.x,
		ctx.base.position.y + config.spawnOffset.y,
		ctx.base.position.z + config.spawnOffset.z,
	};

	// 魔法の設定
	MagicSpawnInfo spawnInfo = ctx.base;
	spawnInfo.direction = ctx.lockOn->GetAimDirection(muzzlePos, 100000.0f); // 後で変えるけどとりあえず長く
	spawnInfo.targetPos = ctx.lockOn->IsLocked()
		? ctx.lockOn->GetPredictedPosition(muzzlePos, 100000.0f)
		: ctx.defaultTarget;

	normalMagic.Emit(ctx.magicManager, spawnInfo, config);
	normalMagic.ResetCharge();
	CameraShakeManager::Instance().Add(0.05f, 0.15f);

	coolDownTimer = coolDown;
}

// 更新処理
void LaserShotSkill::Update(const MagicContext& ctx, float elapsedTime)
{
	if (coolDownTimer > 0.0f) coolDownTimer -= elapsedTime;

	// いつも溜まった状態にしておく
	normalMagic.Update(elapsedTime, ctx.base.owner, config, true);
}

// GUI描画
void LaserShotSkill::DrawGUI()
{
}
