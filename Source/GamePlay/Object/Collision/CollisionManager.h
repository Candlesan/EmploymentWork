#pragma once
#include "GamePlay/Object/Character/Player/Player.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"

class CollisionManager {
public:
	void Update(Player& player, Enemy& enemy);

private:
	// プレイヤーと敵の当たり判定
	void CollisonPlayervsEnemy(Player& player, Enemy& enemy);

	// 武器と敵の当たり判定
	void CollisionPlayerWeaponVsEnemy(Player& player, Enemy& enemy);

	// 敵の武器とプレイヤーの当たり判定
	void CollisionEnemyWeaponVsPlayer(Player& player, Enemy& enemy);

	// 敵と魔法の当たり判定
	void CollisionMagicVsEnemy(Player& player, Enemy& enemy);

	// 敵の魔法とPlayerの当たり判定
	void CollisionEnemyMagicVsPlayer(Player& player, Enemy& enemy);

	// デバック用の当たり判定
	void CollisionDebugSphereVsPlayer(Player& player);
};
