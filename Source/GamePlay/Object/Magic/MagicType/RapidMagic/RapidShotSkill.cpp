#include "RapidShotSkill.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"

#include <imgui.h>


// コンストラクタ
RapidShotSkill::RapidShotSkill()
{
	// 連射魔法を初期化
	rapidMagic.Initialize();
	config.Load(); // Jsonを読み込み

	// エフェクトを設定
	rapidMagic.SetEffect("RapidMagic");
	rapidMagic.SetHitEffect("RapidHit");
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

// GUI描画
void RapidShotSkill::DrawGUI()
{
	if (ImGui::CollapsingHeader(u8"連射魔法"))
	{
		rapidMagic.DrawGUI(config);
	}
}

// デバックプリミティブ描画
void RapidShotSkill::RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer)
{
	if (ctx.lockOn->IsLocked())
	{
		DirectX::XMFLOAT3 predicted = ctx.lockOn->GetPredictedPosition(ctx.base.position, config.speed);
		renderer->DrawSphere(predicted, 1.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
	}
}
