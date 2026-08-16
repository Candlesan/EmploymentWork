#include "NormalMagic.h"
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/Collision/CollisionManager.h"
#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"

#include "System/Effect/EffectManager.h" 
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h" 

#include <imgui.h>

void NormalMagic::Initialize()
{
	CustomEffectManager::Instance().LoadEffectConfig(normalMagic, "Data/Json/Effect/" + normalMagic + ".json");
	CustomEffectManager::Instance().LoadEffectConfig(chargeMagic, "Data/Json/Effect/" + chargeMagic + ".json");
}

// 更新処理
void NormalMagic::Update(float elapsedTime, Character* owner, const NormalMagicConfig& config, bool isButtonHeld)
{
	// 長押し中はチャージ時間を進める
	if (isButtonHeld)
	{
		chargeTimer += elapsedTime;
		isCharging = true; // チャージ中のフラグを立てる
	}
	else
	{
		isCharging = false; // チャージ中のフラグを折る
	}

	// 最大チャージになったか確認
	if (chargeTimer >= config.chargeTimeThreshold)
	{
		isFullCharged = true;
	}

	// 最大チャージ維持中は魔力を減らす
	if (isFullCharged)
	{
		owner->ConsumMagic(config.chargeHoldMagicCost * elapsedTime);
		//　Player側で移動速度を低下させる
	}
}

void NormalMagic::Emit(MagicManager* manager, const MagicSpawnInfo& info, const NormalMagicConfig& config)
{
	float damage = isFullCharged ? config.chargeDamage : config.normalDamage;
	float radius = isFullCharged ? config.chargeRadius : config.normalRadius;
	float range = config.range;

	DirectX::XMFLOAT3 startPos;
	startPos.x = info.position.x + config.spawnOffset.x;
	startPos.y = info.position.y + config.spawnOffset.y;
	startPos.z = info.position.z + config.spawnOffset.z;

	DirectX::XMFLOAT3 centerPos;
	centerPos.x = startPos.x + info.direction.x * (range * 0.5f);
	centerPos.y = startPos.y + info.direction.y * (range * 0.5f);
	centerPos.z = startPos.z + info.direction.z * (range * 0.5f);

	// 位置のオフセット計算
	DirectX::XMFLOAT3 finalPos =
	{
		startPos.x + config.offsetPosition.x,
		startPos.y + config.offsetPosition.y,
		startPos.z + config.offsetPosition.z
	};

	// 方向ベクトル
	float yaw = atan2f(info.direction.x, info.direction.z);
	float pitch = -asinf(info.direction.y);

	// 計算した回転角度
	DirectX::XMFLOAT3 finalAngle = {
		DirectX::XMConvertToDegrees(pitch) + config.offsetAngle.x,
		DirectX::XMConvertToDegrees(yaw) + config.offsetAngle.y,
		0.0f + config.offsetAngle.z
	};

	// スケール計算
	DirectX::XMFLOAT3 finalScale =
	{
		config.offsetScale.x,
		config.offsetScale.y,
		config.offsetScale.z
	};

	// 魔法のパラメータを設定
	MagicProjectile* magic = new MagicProjectile(manager, "");
	magic->SetOwner(info.owner); // 誰が撃ったか

	magic->SetMagicKind(MagicKind::Normal); // 魔法の種類
	magic->SetShape(HitBoxShape::Capsule); // 当たり判定の形状

	// 基本パラメーター
	magic->SetRadius(radius);
	magic->SetHeight(range);
	magic->SetDamage(damage); // 生身へのダメージ
	magic->SetGuardDamage(config.guardDamage); // 防御魔法へのダメージ

	// 魔力消費
	if (info.owner != nullptr)
	{
		const float normalMagicCost = config.normalMagicCost;
		const float chargeMagicCost = config.chargeMagicCost;

		const float normalInterceptBonus = 5.0f; // 通常の迎撃成功時のボーナス
		const float chargeInterceptBonus = 10.0f; // チャージ時の迎撃成功時のボーナス

		if (isCharging)
		{
			magic->SetNormalMagicLevel(NormalMagicLevel::Charged);
			magic->SetInterceptMagicCost(chargeMagicCost);
			magic->SetMagicmagicRefund(chargeMagicCost + chargeInterceptBonus);
		}
		else
		{
			magic->SetNormalMagicLevel(NormalMagicLevel::Normal);
			magic->SetInterceptMagicCost(normalMagicCost);
			magic->SetMagicmagicRefund(normalMagicCost + normalInterceptBonus);
		}
	}

	// 発射遅延/寿命
	magic->SetIsAlive(true);
	magic->SetWaitTimer(0.01f);
	magic->SetLifeTimer(0.1f);

	// 発射
	magic->Launch(info.direction, centerPos, 0.0f);

	if (!isFullCharged)
	{
		// 通常攻撃
		CustomEffectManager::Instance().PlayEffect(normalMagic, finalPos, finalAngle, finalScale);
	}
	else
	{
		// チャージ攻撃
		CustomEffectManager::Instance().PlayEffect(chargeMagic, finalPos, finalAngle, finalScale);
	}
}

void NormalMagic::DrawGUI(NormalMagicConfig& config)
{
	ImGui::Text(u8"チャージ時間 : %f", chargeTimer);
	ImGui::Checkbox(u8"チャージ中か", &isCharging);
	ImGui::Checkbox(u8"フルチャージか", &isFullCharged);
	ImGui::Separator();

	if (ImGui::CollapsingHeader(u8"通常魔法エフェクト調整", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3(u8"位置オフセット (X,Y,Z)", &config.offsetPosition.x, 0.05f);
		ImGui::DragFloat3(u8"角度オフセット (X,Y,Z)", &config.offsetAngle.x, 0.05f);
		ImGui::DragFloat3(u8"拡大率 (X,Y,Z)", &config.offsetScale.x, 0.05f, 0.01f, 10.0f);
	}
}

// チャージ状態を解除する
void NormalMagic::ResetCharge()
{
	chargeTimer = 0.0f;
	isCharging = false;
	isFullCharged = false;
}
