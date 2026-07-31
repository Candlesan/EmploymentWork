#include "NormalMagic.h"
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/Collision/CollisionManager.h"
#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"


#include "System/Effect/EffectManager.h" // Effekseer
#include "System/UI/EffectEditor/EffectEditor.h" // 自作エフェクトエディター(後でマネージャーを作って発生位置とか大きさとか向きをいじれるようなクラスを作っておく)

// 発射
void NormalMagic::Emit(MagicManager* manager, const MagicSpawnInfo& info, const NormalMagicConfig& config)
{
	// 通常攻撃かチャージ攻撃かの判定
	bool isCharged = (info.holdTime >= config.chargeTimeThreshold);

	// チャージ状態によってパラメーターを切り替える
	float damage = isCharged ? config.chargeDamage : config.normalDamage;
	float radius = isCharged ? config.chargeRadius : config.normalRadius;
	float range = config.range;

	// 発射位置の(始点)の計算
	DirectX::XMFLOAT3 startPos;
	startPos.x = info.position.x + config.spawnOffset.x;
	startPos.y = info.position.y + config.spawnOffset.y;
	startPos.z = info.position.z + config.spawnOffset.z;

	// 当たり判定の「中心座標」を計算する
	// ※IntersectCapsuleVsCapsuleはカプセルの中心から上下に伸びる仕様になってるから。
	DirectX::XMFLOAT3 centerPos;
	centerPos.x = startPos.x + info.direction.x * (range * 0.5);
	centerPos.y = startPos.y + info.direction.y * (range * 0.5);
	centerPos.z = startPos.z + info.direction.z * (range * 0.5);

	// ターゲットがいるならヒット判定を出す
	if (info.target != nullptr)
	{
		DirectX::XMFLOAT3 outA, outB;
		bool isHit = Collision::IntersectCapsuleVsCapsule(
			centerPos,							// 攻撃カプセルの中心位置
			info.direction,						// 攻撃カプセルの向き
			range,								// 攻撃カプセルの長さ（射程）
			radius,								// 攻撃カプセルの太さ（半径）
			0.0f,								// 押し出し重さ
			info.target->GetPosition(),         // 相手の位置
			info.target->GetCapsuleDirection(), // 相手の向き
			info.target->GetHeight(),           // 相手の高さ
			info.target->GetRadius(),           // 相手の半径
			1.0f,
			outA, outB);

		// 当たっていたらダメージ
		if (isHit)
		{
			info.target->ApplyDamage(damage, 0.5f, 10.0f);
		}
	}

	//　デバック用にMagicManagerに登録する
	MagicProjectile* magic = new MagicProjectile(manager, "");

	// 位置や長さをセット
	magic->SetShape(HitBoxShape::Capsule);
	magic->SetRadius(radius);
	magic->SetHeight(range);

	// 始点と方向をセット
	magic->Launch(info.direction, centerPos, 0.0f);

	// 即着弾になるから寿命を設定する時はエフェクトの寿命分セットしてあげる
	magic->SetWaitTimer(0.01f);
	magic->SetLifeTimer(0.1f);

	// エフェクトやサウンドの再生（自作ツールのエフェクト/Effekseerのエフェクトを作った時にやる）
	if (isCharged)
	{
		// 太いレーザーエフェクト再生
	}
	else
	{
		// スナイパー一閃エフェクト再生
	}
}
