#include "CollisionManager.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Stage/Stage.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"
#include "GamePlay/Object/Magic/MagicType/RapidMagic/RapidMagic.h"
#include "GamePlay/Object/Magic/MagicType/DefenseMagic/DefenseMagic.h"
#include "GamePlay/Object/Character/Character.h"

// 更新処理
void CollisionManager::Update(Player& player, Enemy& enemy, Stage& stage)
{
	std::vector<Collider> colliders = GatherColliders(player, enemy, stage);
	CheckCollisions(colliders);
}

// 当たった時の処理
void CollisionManager::ResolveCollision(const Collider& a, const Collider& b,
	const DirectX::XMFLOAT3& outPosA, const DirectX::XMFLOAT3& outPosB)
{
	// aが魔法で、bがキャラクターの場合ダメージ
	if (IsMagicTag(a.category) && IsCharacterTag(b.category))
	{

		MagicBase* magic = static_cast<MagicBase*>(a.owner); // void*やからキャストしてあげる必要がある
		Character* target = static_cast<Character*>(b.owner);

		if (target->GetStatus().isAvoid) return; // 回避中は無敵（魔法を消さずにすり抜けさせる）

		// 防御魔法を展開していないときにダメージ
		if (!target->GetIsGaurd())
		{
			target->ApplyDamage(magic->GetDamage());
		}
		else if (target->GetIsGaurd() && magic->GetNormalMagicLevel() == NormalMagicLevel::Charged)
		{
			// 防御魔法を展開してかつチャージ魔法だったら半分のダメージを与える
			target->ApplyDamage(magic->GetDamage() * 0.5f);
		}

		magic->OnTerminate(); // 当たったら魔法を消す

	}
	// aが魔法でbが防御魔法の場合
	else if (IsMagicTag(a.category) && IsDefenseTag(b.category))
	{
		MagicBase* magic = static_cast<MagicBase*>(a.owner); // void*やからキャストしてあげる必要がある
		Character* guardOwner = static_cast<Character*>(b.owner);

		bool wasBroken = guardOwner->GetStatus().isBreak;
		guardOwner->OnGuardHit(magic->GetGuardDamage());  // ガード耐久値を削る

		// このヒットで割れた瞬間だけ、連射魔法を撃った人のヒート値を回復
		if (!wasBroken && guardOwner->GetStatus().isBreak
			&& magic->GetMagicKind() == MagicKind::Rapid && magic->GetOwner())
		{
			magic->GetOwner()->RecoverHeat(magic->GetOwner()->GetStatus().heatRecoverOnBreak);
		}

		if (magic->GetMagicKind() == MagicKind::Normal && magic->GetNormalMagicLevel() == NormalMagicLevel::Charged)
		{
			guardOwner->OnGuardHit(200);
		}

		magic->OnTerminate(); // 魔法は消える
	}
	// aもbもキャラクターの場合押し出し処理
	else if (IsCharacterTag(a.category) && IsCharacterTag(b.category))
	{
		Character* targetA = static_cast<Character*>(a.owner);
		Character* targetB = static_cast<Character*>(b.owner);

		targetA->SetPosition(outPosA); // Collisionが計算してくれた座標をそのまま反映
		targetB->SetPosition(outPosB);
	}
	// aがキャラクターでbがエリアの時の押し戻し処理
	else if (IsCharacterTag(a.category) && IsArenaTag(b.category))
	{
		Character* targetA = static_cast<Character*>(a.owner);

		targetA->SetPosition(outPosA); // Collistionが計算してくれた座標をそのまま反映
	}
	// aが魔法系でbステージの処理
	else if (IsMagicTag(a.category) && IsArenaTag(b.category))
	{
		MagicBase* magic = static_cast<MagicBase*>(a.owner);

		// メモ：通常魔法はステージとヒットしたときに今いる位置からヒットした位置までの長さにする方法が無いから今は連射魔法だけ
		magic->OnTerminate(); // 魔法は消える
	}
}

// このタグはキャラクター本体か？
bool CollisionManager::IsCharacterTag(CollisionTag tag)
{
	return tag == CollisionTag::PlayerBody || tag == CollisionTag::EnemyBody;
}

// このタグはステージか？
bool CollisionManager::IsArenaTag(CollisionTag tag)
{
	return tag == CollisionTag::Arena;
}

// このタグは魔法系か？
bool CollisionManager::IsMagicTag(CollisionTag tag)
{
	return tag == CollisionTag::PlayerNomalMagic || tag == CollisionTag::PlayerRapidMagic
		|| tag == CollisionTag::EnemyNormalMagic || tag == CollisionTag::EnemyRapidMagic;
}

// このタグは防御魔法か？
bool CollisionManager::IsDefenseTag(CollisionTag tag)
{
	return tag == CollisionTag::PlayerDefenseMagic || tag == CollisionTag::EnemyDefenseMagic;
}

// 1フレーム分のColliderをかき集める
std::vector<Collider> CollisionManager::GatherColliders(Player& player, Enemy& enemy, Stage& stage)
{
	std::vector<Collider> colliders;

	// プレイヤー本体
	Collider playerBody;
	playerBody.category = CollisionTag::PlayerBody;
	playerBody.targetMask =
		static_cast<uint32_t>(CollisionTag::EnemyBody)|
		static_cast<uint32_t>(CollisionTag::EnemyNormalMagic)|
		static_cast<uint32_t>(CollisionTag::EnemyRapidMagic)|
		static_cast<uint32_t>(CollisionTag::Arena);
	playerBody.shape = HitBoxShape::Capsule;
	playerBody.pos = player.GetPosition();
	playerBody.dir = player.GetCapsuleDirection();
	playerBody.radius = player.GetRadius();
	playerBody.height = player.GetHeight();
	playerBody.owner = &player;
	colliders.push_back(playerBody);

	// 敵本体
	Collider enemyBody;
	enemyBody.category = CollisionTag::EnemyBody;
	enemyBody.targetMask = 
		static_cast<uint32_t>(CollisionTag::PlayerBody)|
		static_cast<uint32_t>(CollisionTag::PlayerNomalMagic)|
		static_cast<uint32_t>(CollisionTag::PlayerRapidMagic)|
		static_cast<uint32_t>(CollisionTag::Arena);
	enemyBody.shape = HitBoxShape::Capsule;
	enemyBody.pos = enemy.GetPosition();
	enemyBody.dir = enemy.GetCapsuleDirection();
	enemyBody.radius = enemy.GetRadius();
	enemyBody.height = enemy.GetHeight();
	enemyBody.owner = &enemy;
	colliders.push_back(enemyBody);

	// ステージ
	Collider areaCollition;
	areaCollition.category = CollisionTag::Arena;
	areaCollition.targetMask = static_cast<uint32_t>(CollisionTag::None);
	areaCollition.shape = HitBoxShape::Cylinder;
	areaCollition.pos = stage.GetPosition();
	areaCollition.dir = { 0.0f, 1.0f, 0.0f };
	areaCollition.radius = stage.GetRadius();
	areaCollition.height = stage.GetHeight();
	areaCollition.owner = &stage;
	colliders.push_back(areaCollition);

	// プレイヤーの魔法
	MagicManager& playerMagicMgr = player.GetMagicManager();
	for (int i = 0; i < playerMagicMgr.GetMagicCount(); ++i)
	{
		MagicBase* pMagic = playerMagicMgr.getMagic(i);

		if (!pMagic->GetIsAlive()) continue;

		Collider playerMagic; // ループごとに作る

		// 魔法の種類でカテゴリーを分ける
		if (pMagic->GetMagicKind() == MagicKind::Normal)
		{
			playerMagic.category = CollisionTag::PlayerNomalMagic;
			playerMagic.targetMask =
				static_cast<uint32_t>(CollisionTag::EnemyBody) |
				static_cast<uint32_t>(CollisionTag::EnemyDefenseMagic) |
				static_cast<uint32_t>(CollisionTag::EnemyNormalMagic);
		}
		else// MagicKind::Rapid
		{
			playerMagic.category = CollisionTag::PlayerRapidMagic;
			playerMagic.targetMask = static_cast<uint32_t>(CollisionTag::EnemyBody)
				| static_cast<uint32_t>(CollisionTag::EnemyDefenseMagic) |
				static_cast<uint32_t>(CollisionTag::EnemyRapidMagic) |
				static_cast<uint32_t>(CollisionTag::Stage);
		}
		playerMagic.shape = pMagic->GetShape();
		playerMagic.pos = pMagic->GetPosition();
		playerMagic.dir = pMagic->GetDirection();
		playerMagic.radius = pMagic->GetRadius();
		playerMagic.height = pMagic->GetHeight();
		playerMagic.owner = pMagic;

		colliders.push_back(playerMagic); // 1匹分をvectorに積む
	}

	//　敵の魔法
	MagicManager& enemyMagicMgr = enemy.GetMagicManager();
	for (int i = 0; i < enemyMagicMgr.GetMagicCount(); ++i)
	{
		MagicBase* eMagic = enemyMagicMgr.getMagic(i);
		if (!eMagic->GetIsAlive()) continue;

		Collider enemyMagic; // ループごとに作る

		// 魔法の種類でカテゴリーを分ける
		if (eMagic->GetMagicKind() == MagicKind::Normal)
		{
			enemyMagic.category = CollisionTag::EnemyNormalMagic;
			enemyMagic.targetMask = 
				static_cast<uint32_t>(CollisionTag::PlayerBody)|
				static_cast<uint32_t>(CollisionTag::PlayerDefenseMagic) |
				static_cast<uint32_t>(CollisionTag::PlayerNomalMagic) |
				static_cast<uint32_t>(CollisionTag::Arena);

		}
		else// MagicKind::Rapid
		{
			enemyMagic.category = CollisionTag::EnemyRapidMagic;
			enemyMagic.targetMask = 
				static_cast<uint32_t>(CollisionTag::PlayerBody)|
				static_cast<uint32_t>(CollisionTag::PlayerDefenseMagic) |
				static_cast<uint32_t>(CollisionTag::PlayerRapidMagic)|
				static_cast<uint32_t>(CollisionTag::Stage);
		}
		enemyMagic.shape = eMagic->GetShape();
		enemyMagic.pos = eMagic->GetPosition();
		enemyMagic.dir = eMagic->GetDirection();
		enemyMagic.radius = eMagic->GetRadius();
		enemyMagic.height = eMagic->GetHeight();
		enemyMagic.owner = eMagic;

		colliders.push_back(enemyMagic); // 1匹分をvectorに積む
	}

	// プレイヤーの防御魔法
	DefenseMagic& playerDefenseMagic = player.GetDefenseMagic();
	if (playerDefenseMagic.GetIsActive())
	{
		Collider playerGuardCollider;
		playerGuardCollider.category = CollisionTag::PlayerDefenseMagic;
		playerGuardCollider.targetMask = static_cast<uint32_t>(CollisionTag::EnemyNormalMagic)
			| static_cast<uint32_t>(CollisionTag::EnemyRapidMagic);
		playerGuardCollider.shape = HitBoxShape::Sphere;
		playerGuardCollider.pos = playerDefenseMagic.GetDefensePosition();
		playerGuardCollider.radius = playerDefenseMagic.GetDefenseRadius();
		playerGuardCollider.owner = &player;
		colliders.push_back(playerGuardCollider);
	}

	// 敵の防御魔法
	DefenseMagic& enemyDefenseMagic = enemy.GetDefenseMagic();
	if (enemyDefenseMagic.GetIsActive())
	{
		Collider enemyGuardCollider;
		enemyGuardCollider.category = CollisionTag::EnemyDefenseMagic;
		enemyGuardCollider.targetMask = static_cast<uint32_t>(CollisionTag::PlayerNomalMagic)
			| static_cast<uint32_t>(CollisionTag::PlayerRapidMagic);
		enemyGuardCollider.shape = HitBoxShape::Sphere;
		enemyGuardCollider.pos = enemyDefenseMagic.GetDefensePosition();
		enemyGuardCollider.radius = enemyDefenseMagic.GetDefenseRadius();
		enemyGuardCollider.owner = &enemy;
		colliders.push_back(enemyGuardCollider);
	}

	return colliders;
}

// 集めたColliderを総当たりでチェックする
void CollisionManager::CheckCollisions(std::vector<Collider>& colliders)
{
	for (size_t i = 0; i < colliders.size(); ++i)
	{
		for (size_t j = 0; j < colliders.size(); ++j)
		{
			if (i == j) continue; // 自分自身とは判定しない

			Collider& a = colliders[i];
			Collider& b = colliders[j];

			// aがbを狙ってるか?（マスクの判定）
			uint32_t bTag = static_cast<uint32_t>(b.category);
			if ((a.targetMask & bTag) == 0) continue; // aの狙ってるリストの中に、bのタグが1個も含まれてない

			// 形状に応じた当たり判定を取る
			bool hit = false;
			DirectX::XMFLOAT3 outPosA, outPosB;

			if (a.shape == HitBoxShape::Capsule && b.shape == HitBoxShape::Capsule)
			{
				// カプセル同士の当たり判定
				hit = Collision::IntersectCapsuleVsCapsule(
					a.pos, a.dir, a.height, a.radius, 0.1f,
					b.pos, b.dir, b.height, b.radius, 0.1f,
					outPosA, outPosB);
			}
			else if (a.shape == HitBoxShape::Sphere && b.shape == HitBoxShape::Capsule)
			{
				// 球とカプセルの当たり判定
				hit = Collision::IntersectSphereVsCapsule(
					a.pos, a.radius,
					b.pos, b.dir, b.height, b.radius,
					outPosA);
			}
			else if (a.shape == HitBoxShape::Capsule && b.shape == HitBoxShape::Sphere)
			{
				// 球とカプセルの当たり判定
				hit = Collision::IntersectSphereVsCapsule(
					b.pos, b.radius,
					a.pos, a.dir, a.height, a.radius,
					outPosA);
			}
			else if (a.shape == HitBoxShape::Capsule && b.shape == HitBoxShape::Cylinder)
			{
				// 円柱の中にカプセルがあるかの特殊判定
				hit = Collision::ContainCapsuleInCylinder(
					a.pos, a.radius, a.height,
					b.pos, b.radius, b.height,
					outPosA);
			}
			else if (a.shape == HitBoxShape::Sphere && b.shape == HitBoxShape::Cylinder)
			{
				// 円柱の中に球があるかの特殊判定
				hit = Collision::ContainCapsuleInCylinder(
					a.pos, a.radius, 0.0f,
					b.pos, b.radius, b.height,
					outPosA);
			}
			else
			{
				// 球と球の当たり判定
				hit = Collision::IntersectSphereVsSpere(
					a.pos, a.radius,
					b.pos, b.radius,
					outPosA);
			}

			// 当たっていたら判定後の処理に渡す
			if (hit)
			{
				ResolveCollision(a, b, outPosA, outPosB);
			}
		}
	}
}
