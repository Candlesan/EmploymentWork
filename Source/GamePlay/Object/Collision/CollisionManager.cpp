#include "CollisionManager.h"

void CollisionManager::Update(Player& player, Enemy& enemy)
{
	CollisonPlayervsEnemy(player, enemy);
	CollisionPlayerWeaponVsEnemy(player, enemy);
	CollisionEnemyWeaponVsPlayer(player, enemy);
	CollisionMagicVsEnemy(player, enemy);
	CollisionEnemyMagicVsPlayer(player, enemy);
	CollisionDebugSphereVsPlayer(player);
}

// プレイヤーと敵の当たり判定
void CollisionManager::CollisonPlayervsEnemy(Player& player, Enemy& enemy)
{
	// プレイヤーを取得
	DirectX::XMFLOAT3 outPositionA, outPositionB;
	if (Collision::IntersectCapsuleVsCapsule(
		player.GetPosition(),
		player.GetCapsuleDirection(),
		player.GetHeight(),
		player.GetRadius(),
		player.GetWeight(),
		enemy.GetPosition(),
		enemy.GetCapsuleDirection(),
		enemy.GetHeight(),
		enemy.GetRadius(),
		enemy.GetWeight(),
		outPositionA,
		outPositionB
	))
	{
		// 押し出し処理
		player.SetPosition(outPositionA);
		enemy.SetPosition(outPositionB);
	}
}

// 武器と敵の当たり判定
void CollisionManager::CollisionPlayerWeaponVsEnemy(Player& player, Enemy& enemy)
{

	// プレイヤーの現在有効な攻撃判定をすべて取得！
	std::vector<Character::HitBoxInfo> playerHits = player.GetActiveHitBoxes();

	for (const auto& hit : playerHits)
	{
		bool isHit = false;
		DirectX::XMFLOAT3 outPosA, outPosB, outPos;

		if (hit.shape == HitBoxShape::Capsule) {
			isHit = Collision::IntersectCapsuleVsCapsule(
				hit.position, hit.direction, hit.height, hit.radius, 1.0f,
				enemy.GetPosition(), enemy.GetCapsuleDirection(), enemy.GetHeight(), enemy.GetRadius(), enemy.GetWeight(),
				outPosA, outPosB
			);
		}
		else if (hit.shape == HitBoxShape::Sphere) {
			isHit = Collision::IntersectSphereVsCapsule(
				hit.position, hit.radius,
				enemy.GetPosition(), enemy.GetCapsuleDirection(), enemy.GetHeight(), enemy.GetRadius(),
				outPos
			);
		}

		if (isHit) {
			// ダメージ処理
			if (hit.damageRate > 0.0f) {
				AttackResult res = player.CalculateAttackResult(hit.damageRate, hit.poiseRate);
				enemy.SetLastDamage(res.damage);
				enemy.ApplyDamage(res.damage, hit.invincibleTime, res.poiseDamage);
			}
		}
	}
}

// 敵の攻撃とプレイヤーの当たり判定
void CollisionManager::CollisionEnemyWeaponVsPlayer(Player& player, Enemy& enemy)
{

	// 敵の現在有効な攻撃判定をすべて取得！
	std::vector<Character::HitBoxInfo> enemyHits = enemy.GetActiveHitBoxes();

	for (const auto& hit : enemyHits)
	{
		bool isHit = false;
		DirectX::XMFLOAT3 outPos, outPosA, outPosB;

		// 形状に合わせて判定関数を変える
		if (hit.shape == HitBoxShape::Capsule) {
			isHit = Collision::IntersectCapsuleVsCapsule(
				hit.position, hit.direction, hit.height, hit.radius, 1.0f,
				player.GetPosition(), player.GetCapsuleDirection(), player.GetHeight(), player.GetRadius(), 1.0f,
				outPosA, outPosB);
		}
		else if (hit.shape == HitBoxShape::Sphere) {
			isHit = Collision::IntersectSphereVsCapsule(
				hit.position, hit.radius,
				player.GetPosition(), player.GetCapsuleDirection(), player.GetHeight(), player.GetRadius(),
				outPos);
		}

		// 当たっていたらダメージ処理
		if (isHit) {
			player.SetIsHit(true);
			bool isGuarding = player.GetIsGuarding();
			bool isAvoid = player.GetIsAvoid();

			if (isGuarding) player.SetIsHit(true);

			if (!isGuarding && !isAvoid) {
				// hitの中にダメージ情報が入っているのでそのまま使える！
				AttackResult res = enemy.CalculateAttackResult(hit.damageRate, hit.poiseRate);
				//player.SetLastDamage(res.damage);
				player.OnHitEnemyAttack(res.damage, hit.invincibleTime, res.poiseDamage);
			}
		}
	}
}

// 敵と魔法の当たり判定
void CollisionManager::CollisionMagicVsEnemy(Player& player, Enemy& enemy)
{
	MagicManager& magicManager = player.GetMagicManager();

	// 存在する魔法の数だけループ
	for (int i = 0; i < magicManager.GetMagicCount(); ++i)
	{
		MagicBase* magic = magicManager.getMagic(i);

		DirectX::XMFLOAT3 outPos;

		// 魔法と敵の当たり判定
		if (Collision::IntersectSphereVsCapsule(
			magic->GetPosition(), magic->GetRadius(),
			enemy.GetPosition(), enemy.GetCapsuleDirection(), enemy.GetHeight(), enemy.GetRadius(),
			outPos))
		{
			// ダメージを与える
			enemy.ApplyDamage(magic->GetDamage(), 0.3, 10.0f);

			// 魔法自体を消滅させる
			magic->OnTerminate();
		}
	}
}

// 敵の魔法とPlayerの当たり判定
void CollisionManager::CollisionEnemyMagicVsPlayer(Player& player, Enemy& enemy)
{

	MagicManager& enemyMagicManger = enemy.GetMagicManager();

	for (int i = 0; i < enemyMagicManger.GetMagicCount(); ++i)
	{
		// 槍のデータを取り出す
		MagicBase* magic = enemyMagicManger.getMagic(i);
		if (!magic) continue;

		bool isHit = false;

		DirectX::XMFLOAT3 outPosA, outPosB;
		isHit = Collision::IntersectCapsuleVsCapsule(
			magic->GetPosition(),
			magic->GetDirection(),
			magic->GetHeight(),
			magic->GetRadius(),
			0.0f,
			player.GetPosition(),
			player.GetCapsuleDirection(),
			player.GetHeight(),
			player.GetRadius(),
			0.0f,
			outPosA,
			outPosB);

		if (isHit)
		{
			bool isGuarding = player.GetIsGuarding();
			bool isAvoid = player.GetIsAvoid();

			if (!isGuarding && !isAvoid)
			{
				player.ApplyDamage(magic->GetDamage(), 0.5f, 10.0f);
			}

			// 当たった槍は消滅させる
			magic->OnTerminate();
		}
	}
}

// デバック用の当たり判定
void CollisionManager::CollisionDebugSphereVsPlayer(Player& player)
{
	DirectX::XMFLOAT3 debugSpherePos = { 5.0f, 1.0f, 0.0f };
	float debugSphereRadius = 1.0f;

	DirectX::XMFLOAT3 outPos;

	//プレイヤーのカプセルと、この固定の球が触れているかチェック
	if (Collision::IntersectSphereVsCapsule(
		debugSpherePos, debugSphereRadius,
		player.GetPosition(), player.GetCapsuleDirection(), player.GetHeight(), player.GetRadius(),
		outPos))
	{
		if (!player.GetIsGuarding())
		{
			player.OnHitEnemyAttack(10.0f, 5.0f, 0.5f);
		}
	}
}
