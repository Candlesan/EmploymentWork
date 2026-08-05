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
	CustomEffectManager::Instance().LoadEffectConfig(effectName, "Data/Json/Player/Effect/" + effectName + ".json");
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

	if (info.target != nullptr)
	{
		DirectX::XMFLOAT3 outA, outB;
		bool isHit = Collision::IntersectCapsuleVsCapsule(
			centerPos,
			info.direction,
			range,
			radius,
			0.0f,
			info.target->GetPosition(),
			info.target->GetCapsuleDirection(),
			info.target->GetHeight(),
			info.target->GetRadius(),
			1.0f,
			outA, outB);

		if (isHit)
		{
			info.target->ApplyDamage(damage, 0.5f, 10.0f);
		}
	}

	MagicProjectile* magic = new MagicProjectile(manager, "");
	magic->SetShape(HitBoxShape::Capsule);
	magic->SetRadius(radius);
	magic->SetHeight(range);
	magic->Launch(info.direction, centerPos, 0.0f);
	magic->SetWaitTimer(0.01f);
	magic->SetLifeTimer(0.1f);

	// ★ ① 位置のオフセット計算 (config.offsetPosition を使用)
	DirectX::XMFLOAT3 finalPos =
	{
		startPos.x + config.offsetPosition.x,
		startPos.y + config.offsetPosition.y,
		startPos.z + config.offsetPosition.z
	};

	// ★ ② 方向ベクトル (info.direction) から Yaw (Y回転) と Pitch (X回転) を計算！
	float yaw = atan2f(info.direction.x, info.direction.z);
	float pitch = -asinf(info.direction.y);

	// ★ ③ 計算した回転角度（度数法） ＋ オフセット角度 (config.offsetAngle)
	DirectX::XMFLOAT3 finalAngle = {
		DirectX::XMConvertToDegrees(pitch) + config.offsetAngle.x,
		DirectX::XMConvertToDegrees(yaw) + config.offsetAngle.y,
		0.0f + config.offsetAngle.z
	};

	// ★ ④ スケール計算 (config.offsetScale を使用)
	DirectX::XMFLOAT3 finalScale =
	{
		config.offsetScale.x,
		config.offsetScale.y,
		config.offsetScale.z
	};

	if (!isCharged)
	{
		// 最終座標と角度でエフェクトを再生！
		CustomEffectManager::Instance().PlayEffect(effectName, finalPos, finalAngle, finalScale);
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