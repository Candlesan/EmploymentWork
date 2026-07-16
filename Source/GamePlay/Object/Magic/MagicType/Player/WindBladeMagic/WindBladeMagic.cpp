#include "WindBladeMagic.h"
#include "System/Core/Mathf.h"
#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"


// 球を生成する関数
void WindBladeMagic::Emit(MagicManager* manager, const MagicSpawnInfo& info, const WindBladeConfig& config) {
	// 前方向
	DirectX::XMFLOAT3 dir;
	dir.x = sinf(info.angleY);
	dir.y = 0.0f;
	dir.z = cosf(info.angleY);
	// 発射位置（仮でプレイヤーの腰当たり）
	DirectX::XMFLOAT3 pos;
	pos.x = info.position.x + config.spawnOffset.x;
	pos.y = info.position.y + config.spawnOffset.y;
	pos.z = info.position.z + config.spawnOffset.z;

	// 発射
	MagicProjectile* magic = new MagicProjectile(manager, "Data/Effect/WindBleade/WindBlead.efk");
	magic->SetDamage(config.damage);
	magic->SetSpeed(config.speed);
	magic->SetLifeTimer(config.lifeTimer);

	// まっすぐ飛ぶ部品を作って、弾本体にハメ込む！
	magic->SetMoveComponent(std::make_unique<StraightMove>());

	magic->Launch(dir, pos, config.waitTimer);
}