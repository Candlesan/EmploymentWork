#include "RapidMagic.h"
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"

#include "System/Effect/EffectManager.h" 
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h" 

#include <imgui.h>


// 初期化
void RapidMagic::Initialize()
{
	// エフェクト作った時にロードする
	CustomEffectManager::Instance().LoadEffectConfig(rapidMagic, "Data/Json/Effect/RapidFireMagic.json");
	CustomEffectManager::Instance().LoadEffectConfig(magicCircle, "Data/Json/Effect/MagicCircle_Fast.json");
}

// 魔法を生成
void RapidMagic::Emit(MagicManager* manager, const MagicSpawnInfo& info, const RapidMagicConfig& config)
{

	DirectX::XMFLOAT3 localOffset = config.spawnOffsets[currentSpawnIndex];

	// オフセットの合成
	DirectX::XMFLOAT3 totalOffset = {
		config.spawnOffset.x + localOffset.x,
		config.spawnOffset.y + localOffset.y,
		config.spawnOffset.z + localOffset.z,
	};

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

	// プレイヤーの位置・向きからワールド座標を求める
	DirectX::XMFLOAT3 startPos = {
		info.position.x + (right.x * totalOffset.x) + (forward.x * totalOffset.z),
		info.position.y + totalOffset.y,
		info.position.z + (right.z * totalOffset.x) + (forward.z * totalOffset.z),
	};	 

	// 弾を生成
	MagicProjectile* magic = new MagicProjectile(manager, rapidMagic);

	magic->SetOwner(info.owner); // 誰が撃ったか


	// 直進移動コンポーネントセット
	magic->SetMoveComponent(std::make_unique<StraightMove>());

	magic->SetMagicKind(MagicKind::Rapid); // 連射魔法
	magic->SetShape(HitBoxShape::Sphere);  // 当たり判定の形状

	magic->SetRadius(config.hitRadius);			// 当たり判定の半径
	magic->SetDamage(0.01f);					// 生身へのダメージ
	magic->SetGuardDamage(config.gaurdDamage);	// 防御魔法へのダメージ

	if (info.owner != nullptr)
	{
		const float magicCost =
			info.owner->GetStatus().rapidFireMagicCost;

		const float interceptBonus = 1.0f; // 迎撃成功時のボーナス

		magic->SetInterceptMagicCost(magicCost);
		magic->SetMagicmagicRefund(magicCost + interceptBonus);
	}

	magic->SetSpeed(config.speed);				// 弾速
	magic->SetLifeTimer(config.lifeTimer);		// 射程(生存時間)
	magic->SetWaitTimer(0.0f);
	magic->SetIsAlive(true);

	// 発射
	magic->Launch(info.direction, startPos, 0.0f);

	// 次に発射のためにインデックスを１つ進める
	currentSpawnIndex = (currentSpawnIndex + 1) % 6;
}

// 更新処理
void RapidMagic::Update(float elapsedTime, MagicManager* manager, const MagicSpawnInfo& info, const RapidMagicConfig& config, bool isTriggered)
{
	if (isTriggered)
	{
		rapidTimer -= elapsedTime;

		if (rapidTimer <= 0.0f)
		{
			Emit(manager, info, config);
			rapidTimer = config.rapidInterval; // 内部でタイマーリセット
		}
	}
}

// GUI描画
void RapidMagic::DrawGUI(RapidMagicConfig& config)
{
	if (ImGui::TreeNode(u8"スポーン位置調整 (6箇所)"))
	{
		for (int i = 0; i < 6; ++i)
		{
			std::string label = u8"ポイント " + std::to_string(i + 1);
			ImGui::DragFloat3(label.c_str(), &config.spawnOffsets[i].x, 0.05f);
		}
		ImGui::TreePop();
	}
}
