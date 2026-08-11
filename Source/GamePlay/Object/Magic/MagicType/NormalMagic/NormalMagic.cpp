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
	CustomEffectManager::Instance().LoadEffectConfig(normalMagic, "Data/Json/Player/Effect/" + normalMagic + ".json");
	CustomEffectManager::Instance().LoadEffectConfig(chargeMagic, "Data/Json/Player/Effect/" + chargeMagic + ".json");
}

void NormalMagic::Emit(MagicManager* manager, const MagicSpawnInfo& info, const NormalMagicConfig& config)
{
	bool isCharged = (info.holdTime >= config.chargeTimeThreshold);

	float damage = isCharged ? config.chargeDamage : config.normalDamage;
	float radius = isCharged ? config.chargeRadius : config.normalRadius;
	float range = config.range;

	DirectX::XMFLOAT3 startPos;
	startPos.x = info.position.x + config.spawnOffset.x;
	startPos.y = info.position.y + config.spawnOffset.y;
	startPos.z = info.position.z + config.spawnOffset.z;

	DirectX::XMFLOAT3 centerPos;
	centerPos.x = startPos.x + info.direction.x * (range * 0.5f);
	centerPos.y = startPos.y + info.direction.y * (range * 0.5f);
	centerPos.z = startPos.z + info.direction.z * (range * 0.5f);

	//if (info.target != nullptr)
	//{
	//	DirectX::XMFLOAT3 outA, outB;
	//	bool isHit = Collision::IntersectCapsuleVsCapsule(
	//		centerPos,
	//		info.direction,
	//		range,
	//		radius,
	//		0.0f,
	//		info.target->GetPosition(),
	//		info.target->GetCapsuleDirection(),
	//		info.target->GetHeight(),
	//		info.target->GetRadius(),
	//		1.0f,
	//		outA, outB);

	//	if (isHit)
	//	{
	//		info.target->ApplyDamage(damage);
	//	}
	//}

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
	magic->SetMagicKind(MagicKind::Normal); // 魔法の種類
	magic->SetShape(HitBoxShape::Capsule); // 当たり判定の形状
	magic->SetRadius(radius);
	magic->SetHeight(range);
	magic->SetDamage(damage); // 生身へのダメージ
	magic->SetGuardDamage(config.guardDamage); // 防御魔法へのダメージ
	magic->SetWaitTimer(0.01f);
	magic->SetLifeTimer(0.1f);
	magic->Launch(info.direction, centerPos, 0.0f);

	if (!isCharged)
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
	if (ImGui::CollapsingHeader(u8"通常魔法エフェクト調整"))
	{
		ImGui::DragFloat3(u8"位置オフセット (X,Y,Z)", &config.offsetPosition.x, 0.05f);
		ImGui::DragFloat3(u8"角度オフセット (X,Y,Z)", &config.offsetAngle.x, 0.05f);
		ImGui::DragFloat3(u8"拡大率 (X,Y,Z)", &config.offsetScale.x, 0.05f, 0.01f, 10.0f);
	}
}