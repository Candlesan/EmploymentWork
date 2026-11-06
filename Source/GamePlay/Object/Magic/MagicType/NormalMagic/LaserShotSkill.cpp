#include "LaserShotSkill.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"
#include "GamePlay/Object/CameraShake/CameraShake.h"
#include "GamePlay/Object/Character/Character.h"

#include <imgui.h>


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
	if (!ctx.canCast) return;         // 操作できない
	if (coolDownTimer > 0.0f) return; // クールダウン中
	if (isShot) return;               // もう予約している

	// 予約をする
	isShot = true;
	reserveTimer = 0.0f;
}

// 更新処理
void LaserShotSkill::Update(const MagicContext& ctx, float elapsedTime)
{
	if (coolDownTimer > 0.0f) coolDownTimer -= elapsedTime;

	// 予約時間を更新する
	if (isShot)
	{
		reserveTimer += elapsedTime;
		if (reserveTimer >= 2.0f) isShot = false;
	}

	// 予約があるかつshotのイベントが呼ばれたら撃つ
	if (isShot && ctx.isShotEvent)
	{
		Fire(ctx);
		isShot = false; // 予約をけす
	}

	// いつも溜まった状態にしておく
	normalMagic.Update(elapsedTime, ctx.base.owner, config, true);
}

// GUI描画
void LaserShotSkill::DrawGUI()
{
	// 魔法関係
	if (ImGui::CollapsingHeader(u8"通常魔法"))
	{
		if (ImGui::Button(u8"保存##Normal"))
		{
			config.Save();
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"読み込み##Normal"))
		{
			config.Load(); // JSONから読み込み
		}
		ImGui::Separator();

		ImGui::Text(u8"(通常撃ち / チャージ切り替え時間: %.2f秒)", config.chargeTimeThreshold);
		ImGui::DragFloat(u8"切り替え閾値時間", &config.chargeTimeThreshold, 0.05f, 0.1f, 3.0f);
		ImGui::DragFloat(u8"射程距離 (Range)", &config.range, 1.0f, 10.0f, 300.0f);
		ImGui::Separator();

		ImGui::Text(u8"--- 通常撃ち ---");
		ImGui::DragFloat(u8"通常ダメージ", &config.normalDamage, 1.0f);
		ImGui::DragFloat(u8"通常判定の太さ (半径)", &config.normalRadius, 0.1f, 0.1f, 5.0f);
		ImGui::DragFloat(u8"通常の魔力消費量", &config.normalMagicCost, 0.1f);
		ImGui::Separator();

		ImGui::Text(u8"--- チャージ撃ち  ---");
		ImGui::DragFloat(u8"チャージダメージ", &config.chargeDamage, 1.0f);
		ImGui::DragFloat(u8"チャージ判定の太さ (半径)", &config.chargeRadius, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat(u8"チャージの魔力消費量", &config.chargeMagicCost, 0.1f);
		ImGui::DragFloat(u8"フルチャージの維持中の魔力消費量", &config.chargeHoldMagicCost, 0.1f);
		ImGui::Separator();

		ImGui::DragFloat3(u8"手元オフセット##normal", &config.spawnOffset.x, 0.1f);
		ImGui::Separator();

		normalMagic.DrawGUI(config);
	}
}

// 魔法を撃つ
void LaserShotSkill::Fire(const MagicContext& ctx)
{
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
