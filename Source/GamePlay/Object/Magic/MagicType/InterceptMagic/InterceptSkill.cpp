#include "InterceptSkill.h"

#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"

#include "GamePlay/Object/CameraShake/CameraShake.h"
#include "GamePlay/Object/Character/Character.h"

// コンストラクタ
InterceptSkill::InterceptSkill()
{
	// 連射魔法を初期化
	rapidMagic.Initialize();
	config.Load(); // Jsonを読み込み

	// エフェクトを設定
	rapidMagic.SetEffect("InterceptShot");
	rapidMagic.SetHitEffect("Intercept");

	// 迎撃の設定
	config.damage		= 0.0f;  // キャラクターに与えるダメージ
	config.gaurdDamage	= 0.0f;	  // 防御魔法に与えるダメージ
	config.hitRadius	= 0.25f;  // 当たり判定の大きさ
	config.speed		= 80.0f; // 弾速

}

// 長押し
void InterceptSkill::OnHold(const MagicContext& ctx, float elapsedTime)
{
	if (!ctx.canCast) return;

	// 迎撃の中心位置
	DirectX::XMFLOAT3 centerPos = {
		ctx.base.position.x + centerOffset.x,
		ctx.base.position.y + centerOffset.y,
		ctx.base.position.z + centerOffset.z
	};

	// 一番近い魔法を取得する
	auto targets = ctx.opponentMagicManager->FindNearProjectiles(ctx.base.owner, centerPos, interceptRadius);
	if (targets.empty()) return; 


	MagicBase* targetMagic = targets[0];
	
	// 迎撃する方向を計算する
	DirectX::XMVECTOR magicPos = DirectX::XMLoadFloat3(&targetMagic->GetPosition());
	DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&ctx.base.position);

	DirectX::XMFLOAT3 interceptDir{};
	DirectX::XMStoreFloat3(&interceptDir, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(magicPos, playerPos)));

	// 魔法の設定
	MagicSpawnInfo spawnInfo = ctx.base;
	spawnInfo.direction = interceptDir;
	spawnInfo.targetPos = targetMagic->GetPosition();

	rapidMagic.Update(elapsedTime, ctx.magicManager, spawnInfo, config, true);

	// 敵が連射魔法なら撃ち落とす
	if (targetMagic->GetMagicKind() == MagicKind::Rapid && targetMagic->GetIsAlive())
	{
		ctx.base.owner->RecoverMagic(targetMagic->GetMagicmagicRefund());
		CustomEffectManager::Instance().PlayBurst("Intercept", targetMagic->GetPosition(), { 0, 0, 0 }, { 1, 1, 1 });
		CameraShakeManager::Instance().Add(targetMagic->GetPosition(), 10.0f, 0.5f, 0.1f);
		targetMagic->OnTerminate();
	}
}

// GUI描画
void InterceptSkill::DrawGUI()
{
}

// デバックプリミティブ描画
void InterceptSkill::RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer)
{
	if (!ctx.isCounterActive) return; // 迎撃中だけ描く

	DirectX::XMFLOAT3 center = {
		ctx.base.position.x + centerOffset.x,
		ctx.base.position.y + centerOffset.y,
		ctx.base.position.z + centerOffset.z
	};

	// 迎撃範囲の球
	renderer->DrawSphere(center, interceptRadius, { 0.0f, 1.0f, 1.0f, 0.5f });
}
