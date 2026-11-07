#include "ShotgunMagic.h"
#include <imgui.h>

#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h" 

#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"
#include "GamePlay/Object/Character/Character.h"


// 初期化
void ShotgunMagic::Initialize()
{
	auto& effectManager = CustomEffectManager::Instance();

	// エフェクト読み込み（仮で連射魔法のエフェクトを使う）
	effectManager.LoadEffectConfig("RapidMagic", "Data/Json/Effect/RapidFireMagic.json");
	effectManager.LoadEffectConfig("RapidHit", "Data/Json/Effect/RapidHit.json");
	effectManager.GetOrCreateResident("RapidHit");
}

// 発射関数
void ShotgunMagic::Emit(MagicManager* manager, const MagicSpawnInfo& info, const ShotgunMagicConfig& config)
{
	// オフセットの合成
	DirectX::XMFLOAT3 startPos = {
		info.position.x + config.spawnOffset.x,
		info.position.y + config.spawnOffset.y,
		info.position.z + config.spawnOffset.z
	};

	// 発射方向を計算する
	DirectX::XMVECTOR S = DirectX::XMLoadFloat3(&startPos);
	DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&info.targetPos);
	DirectX::XMVECTOR V = DirectX::XMVectorSubtract(T, S);
	DirectX::XMVECTOR Dir = DirectX::XMVector3Normalize(V);

	DirectX::XMFLOAT3 dir;
	DirectX::XMStoreFloat3(&dir, Dir);

	DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMVECTOR Right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(Up, Dir));
	Up = DirectX::XMVector3Cross(Dir, Right);

	for (int i = 0; i < config.pelletCount; ++i)
	{
		// 拡散を入れる
		std::uniform_real_distribution<float> spread(-config.spreadAngle, config.spreadAngle);
		float yaw = DirectX::XMConvertToRadians(spread(rng)); // 左右に回す角度
		float pitch = DirectX::XMConvertToRadians(spread(rng)); // 上下に回す角度

		DirectX::XMMATRIX R = DirectX::XMMatrixRotationAxis(Up, yaw) * DirectX::XMMatrixRotationAxis(Right, pitch);
		DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(Dir, R)));

		// 弾を生成
		MagicProjectile* magic = new MagicProjectile(manager, "RapidMagic");

		magic->SetOwner(info.owner); // 誰が撃ったか

		// 直進移動コンポーネントセット
		magic->SetMoveComponent(std::make_unique<StraightMove>());

		magic->SetMagicKind(MagicKind::Shotgun); // ショットガン
		magic->SetShape(HitBoxShape::Sphere);  // 当たり判定の形状

		magic->SetRadius(config.hitRadius);			// 当たり判定の半径
		magic->SetDamage(config.damage);			// 生身へのダメージ
		magic->SetGuardDamage(config.guardDamage);	// 防御魔法へのダメージ
		magic->SetFalloff(config.falloffStart, config.falloffEnd, config.falloffMinRate); // 距離減衰
		magic->SetSpeed(config.speed);				// 弾速
		magic->SetLifeTimer(config.lifeTimer);		// 射程(生存時間)
		magic->SetWaitTimer(0.0f);
		magic->SetIsAlive(true);
		magic->SetHitEffect("RapidHit", 1.0f);

		if (info.owner != nullptr)

		// 発射
		magic->Launch(dir, startPos, 0.0f);
		info.owner->ConsumMagic(config.magicCost);
	}
}

// GUI描画
void ShotgunMagic::DrawGUI(ShotgunMagicConfig& config)
{
	if (ImGui::Button(u8"保存##Shotgun"))
	{
		config.Save();
	}
	ImGui::SameLine();
	if (ImGui::Button(u8"読み込み##Shotgun"))
	{
		config.Load(); // JSONから読み込み
	}
	ImGui::Separator();

	ImGui::DragFloat(u8"発射間隔", &config.interval, 0.01f);
	ImGui::DragFloat(u8"寿命", &config.lifeTimer, 0.01f);
	ImGui::DragFloat(u8"拡散", &config.spreadAngle, 0.1f);
	ImGui::DragInt(u8"同時に打ち出す数", &config.pelletCount, 1);
	ImGui::Separator();

	ImGui::DragFloat(u8"生身へのダメージ", &config.damage, 0.1f);
	ImGui::DragFloat(u8"防御魔法へのダメージ", &config.guardDamage, 0.1f);
	ImGui::DragFloat(u8"減り始め", &config.falloffStart, 0.1f);
	ImGui::DragFloat(u8"減り終わり", &config.falloffEnd, 0.1f);
	ImGui::DragFloat(u8"最低の倍率", &config.falloffMinRate, 0.1f);
	ImGui::Separator();

	ImGui::DragFloat(u8"魔法の速度", &config.speed, 0.1f);
	ImGui::DragFloat(u8"魔力消費量", &config.magicCost, 0.1f);
	ImGui::DragFloat(u8"当たり判定の大きさ", &config.hitRadius, 0.01f);
	ImGui::Separator();


	ImGui::DragFloat3(u8"手元オフセット##rapid", &config.spawnOffset.x, 0.1f);
	ImGui::Separator();
}
