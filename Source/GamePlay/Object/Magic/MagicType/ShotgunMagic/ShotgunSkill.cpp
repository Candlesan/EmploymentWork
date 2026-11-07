#include "ShotgunSkill.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"

#include <imgui.h>


// コンストラクタ
ShotgunSkill::ShotgunSkill()
{
	// 連射魔法を初期化
	shotgunMagic.Initialize();
	config.Load(); // Jsonを読み込み
}

// 長押し
void ShotgunSkill::OnHold(const MagicContext& ctx, float elapsedTime)
{
	if (!ctx.canCast || ctx.isCounterActive) return; // 迎撃中または操作不可
	if (coolDownTimer > 0.0f) return;

	// 狙う先
	MagicSpawnInfo spawnInfo = ctx.base;
	spawnInfo.targetPos = ctx.lockOn->IsLocked()
		? ctx.lockOn->GetPredictedPosition(ctx.base.position, config.speed)
		: ctx.defaultTarget;

	shotgunMagic.Emit(ctx.magicManager, spawnInfo, config);

	coolDownTimer = config.interval;
}

// 更新処理
void ShotgunSkill::Update(const MagicContext& ctx, float elapsedTime)
{
	if (coolDownTimer > 0.0f) coolDownTimer -= elapsedTime;
}

// GUI描画
void ShotgunSkill::DrawGUI()
{
	if (ImGui::CollapsingHeader(u8"ショットガン"))
	{
		ImGui::Text(u8"クールダウン: %.2f", coolDownTimer);
		shotgunMagic.DrawGUI(config);
	}
}
