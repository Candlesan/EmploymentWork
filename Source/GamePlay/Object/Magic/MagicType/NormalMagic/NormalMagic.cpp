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
	CustomEffectManager::Instance().LoadEffectConfig(magicCircleFast, "Data/Json/Effect/MagicCircle_Fast.json");
	CustomEffectManager::Instance().LoadEffectConfig(magicCircleLong, "Data/Json/Effect/MagicCircle_Long.json");
	CustomEffectManager::Instance().LoadEffectConfig("NormalMagicHit", "Data/Json/Effect/NormalMagicHit.json");
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

	// 水平方向の前方向
	float len = sqrtf(info.direction.x * info.direction.x + info.direction.z * info.direction.z);
	DirectX::XMFLOAT3 forward = { 0.0f, 0.0f, 0.0f };
	if (len > 0.0001f)
	{
		// 正規化する
		forward.x = info.direction.x / len;
		forward.z = info.direction.z / len;
	}

	// 前方向から右方向ベクトルを作る
	DirectX::XMFLOAT3 right = { forward.z, 0.0f, -forward.x };

	// ================ 通常レーザーの計算 =================

	// レーザー位置のオフセット計算
	DirectX::XMFLOAT3 laserFinalPos =
	{
		startPos.x + (right.x * config.laserOffsetPosition.x) + (forward.x * config.laserOffsetPosition.z),
		startPos.y + config.laserOffsetPosition.y,
		startPos.z + (right.z * config.laserOffsetPosition.x) + (forward.z * config.laserOffsetPosition.z) 
	};

	// 方向ベクトル
	float yaw = atan2f(info.direction.x, info.direction.z);
	float pitch = -asinf(info.direction.y);

	// レーザーの計算した回転角度
	DirectX::XMFLOAT3 laserFinalAngle = {
		DirectX::XMConvertToDegrees(pitch) + config.laserOffsetAngle.x,
		DirectX::XMConvertToDegrees(yaw) + config.laserOffsetAngle.y,
		0.0f + config.laserOffsetAngle.z
	};

	// レーザーのスケール計算
	DirectX::XMFLOAT3 laserFinalScale =
	{
		config.laserOffsetScale.x,
		config.laserOffsetScale.y,
		config.laserOffsetScale.z
	};

	// ================ チャージ魔法の計算 =================

	// チャージ魔法位置のオフセット計算
	DirectX::XMFLOAT3 laserChargeFinalPos =
	{
		startPos.x + (right.x * config.laserChargeOffsetPosition.x) + (forward.x * config.laserChargeOffsetPosition.z),
		startPos.y + config.laserChargeOffsetPosition.y,
		startPos.z + (right.z * config.laserChargeOffsetPosition.x) + (forward.z * config.laserChargeOffsetPosition.z)
	};

	// チャージ魔法の計算した回転角度
	DirectX::XMFLOAT3 laserChargeFinalAngle = {
		DirectX::XMConvertToDegrees(pitch) + config.laserChargeOffsetAngle.x,
		DirectX::XMConvertToDegrees(yaw) + config.laserChargeOffsetAngle.y,
		0.0f + config.laserChargeOffsetAngle.z
	};

	// チャージ魔法のスケール計算
	DirectX::XMFLOAT3 laserChargeFinalScale =
	{
		config.laserChargeOffsetScale.x,
		config.laserChargeOffsetScale.y,
		config.laserChargeOffsetScale.z
	};

	// ================ 魔法陣の計算 =================

	// 魔法陣の位置オフセット
	DirectX::XMFLOAT3 circleFinalPos =
	{
		info.position.x + (right.x * config.circleOffsetPosition.x) + (forward.x * config.circleOffsetPosition.z),
		info.position.y + config.circleOffsetPosition.y,
		info.position.z + (right.z * config.circleOffsetPosition.x) + (forward.z * config.circleOffsetPosition.z)
	};

	// 魔法陣の計算した回転角度
	DirectX::XMFLOAT3 circleFinalAngle = {
		DirectX::XMConvertToDegrees(pitch) + config.circleOffsetAngle.x,
		DirectX::XMConvertToDegrees(yaw) + config.circleOffsetAngle.y,
		0.0f + config.circleOffsetAngle.z
	};

	// 魔法陣のスケール計算
	DirectX::XMFLOAT3 circleFinalScale =
	{
		config.circleOffsetScale.x,
		config.circleOffsetScale.y,
		config.circleOffsetScale.z
	};

	// 魔法のパラメータを設定
	MagicProjectile* magic = new MagicProjectile(manager, "");
	magic->SetOwner(info.owner); // 誰が撃ったか

	magic->SetMagicKind(MagicKind::Normal); // 魔法の種類
	magic->SetShape(HitBoxShape::Capsule); // 当たり判定の形状

	// 基本パラメーター
	magic->SetRadius(radius);
	magic->SetGrow(range, rangeDuration);
	magic->SetDamage(damage); // 生身へのダメージ
	magic->SetGuardDamage(config.guardDamage); // 防御魔法へのダメージ
	magic->SetHitEffect("NormalMagicHit");
	magic->SetHitShake(0.15f, 0.4f, 15.0f);

	// 魔力消費
	if (info.owner != nullptr)
	{

		if (isFullCharged)
		{
			magic->SetNormalMagicLevel(NormalMagicLevel::Charged);
			info.owner->ConsumMagic(config.chargeMagicCost);
		}
		else
		{
			magic->SetNormalMagicLevel(NormalMagicLevel::Normal);
			info.owner->ConsumMagic(config.normalMagicCost);
		}
	}

	// 発射遅延/寿命
	magic->SetIsAlive(true);
	magic->SetWaitTimer(0.01f);
	magic->SetLifeTimer(0.5f);

	// 発射
	magic->Launch(info.direction, startPos, 0.0f);

	// エフェクトの再生
	if (!isFullCharged)
	{
		// 通常攻撃
		CustomEffectManager::Instance().PlayEffect(normalMagic, laserFinalPos, laserFinalAngle, laserFinalScale);
		CustomEffectManager::Instance().PlayEffect(magicCircleFast, circleFinalPos, circleFinalAngle, circleFinalScale);
	}
	else
	{
		// チャージ攻撃
		CustomEffectManager::Instance().PlayEffect(chargeMagic, laserChargeFinalPos, laserChargeFinalAngle, laserChargeFinalScale);

		circleFinalScale.x *= 2.0f;
		circleFinalScale.y *= 2.0f;
		circleFinalScale.z *= 2.0f;

		CustomEffectManager::Instance().PlayEffect(magicCircleLong, circleFinalPos, circleFinalAngle, circleFinalScale);

	}

}

void NormalMagic::DrawGUI(NormalMagicConfig& config)
{
	ImGui::Text(u8"チャージ時間 : %f", chargeTimer);
	ImGui::Checkbox(u8"チャージ中か", &isCharging);
	ImGui::Checkbox(u8"フルチャージか", &isFullCharged);
	ImGui::Separator();

	if (ImGui::CollapsingHeader(u8"通常魔法エフェクト", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3(u8"位置##Normal", &config.laserOffsetPosition.x, 0.05f);
		ImGui::DragFloat3(u8"角度##Normal", &config.laserOffsetAngle.x, 0.05f);
		ImGui::DragFloat3(u8"拡大率##Normal", &config.laserOffsetScale.x, 0.05f, 0.01f, 10.0f);
	}
	ImGui::Separator();

	if (ImGui::CollapsingHeader(u8"チャージ魔法エフェクト", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3(u8"位置##Charge", &config.laserChargeOffsetPosition.x, 0.05f);
		ImGui::DragFloat3(u8"角度##Charge", &config.laserChargeOffsetAngle.x, 0.05f);
		ImGui::DragFloat3(u8"拡大率##Charge", &config.laserChargeOffsetScale.x, 0.05f, 0.01f, 10.0f);
	}
	ImGui::Separator();

	if (ImGui::CollapsingHeader(u8"魔法陣エフェクト", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3(u8"位置オフセット##Circle", &config.circleOffsetPosition.x, 0.05f);
		ImGui::DragFloat3(u8"角度オフセット##Circle", &config.circleOffsetAngle.x, 0.05f);
		ImGui::DragFloat3(u8"拡大率##Circle", &config.circleOffsetScale.x, 0.05f, 0.01f, 10.0f);
	}
}

// チャージ状態を解除する
void NormalMagic::ResetCharge()
{
	chargeTimer = 0.0f;
	isCharging = false;
	isFullCharged = false;
}
