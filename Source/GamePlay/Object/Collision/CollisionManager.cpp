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
	colliders.clear(); // 前フレームの分を消す

	colliders = GatherColliders(player, enemy, stage);
	CheckCollisions(colliders);

	// 魔法とステージはレイで判定する
	CheckMagicVsStage(player.GetMagicManager(), stage);
	CheckMagicVsStage(enemy.GetMagicManager(), stage);
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

		if (!target->GetIsGaurd())magic->PlayHitEffect(ComputeHitPosition(a, b));
		magic->OnTerminate(); // 当たったら魔法を消す

	}
	// aが魔法でbが防御魔法の場合
	else if (IsMagicTag(a.category) && IsDefenseTag(b.category))
	{
		MagicBase* magic = static_cast<MagicBase*>(a.owner); // void*やからキャストしてあげる必要がある
		Character* guardOwner = static_cast<Character*>(b.owner);

		bool wasBroken = guardOwner->GetStatus().isBreak;
		guardOwner->OnGuardHit(magic->GetGuardDamage());  // ガード耐久値を削る

		if (magic->GetMagicKind() == MagicKind::Normal && magic->GetNormalMagicLevel() == NormalMagicLevel::Charged)
		{
			guardOwner->OnGuardHit(200);
		}

		if (magic->GetMagicKind() == MagicKind::Rapid) magic->PlayHitEffect(a.pos);

		magic->OnTerminate(); // 魔法は消える
	}
	// aもbもキャラクターの場合押し出し処理
	else if (IsCharacterTag(a.category) && IsCharacterTag(b.category))
	{
		Character* targetA = static_cast<Character*>(a.owner);
		Character* targetB = static_cast<Character*>(b.owner);

		// 判定の中心が動かされた量だけ足す（オフセット込みの座標をそのまま入れない）
		DirectX::XMFLOAT3 posA = targetA->GetPosition();
		DirectX::XMFLOAT3 posB = targetB->GetPosition();
		targetA->SetPosition({ posA.x + (outPosA.x - a.pos.x), posA.y + (outPosA.y - a.pos.y), posA.z + (outPosA.z - a.pos.z) });
		targetB->SetPosition({ posB.x + (outPosB.x - b.pos.x), posB.y + (outPosB.y - b.pos.y), posB.z + (outPosB.z - b.pos.z) });
	}
	// aがキャラクターでbがエリアの時の押し戻し処理
	else if (IsCharacterTag(a.category) && IsArenaTag(b.category))
	{
		Character* targetA = static_cast<Character*>(a.owner);

		// 判定の中心がどれだけ動かされたか（オフセットに関係なく使える）
		DirectX::XMFLOAT3 pushA = { outPosA.x - a.pos.x, outPosA.y - a.pos.y, outPosA.z - a.pos.z };
		DirectX::XMFLOAT3 posA = targetA->GetPosition();
		targetA->SetPosition({ posA.x + pushA.x, posA.y + pushA.y, posA.z + pushA.z });
	}
	// aが魔法系でbがエリアの処理
	else if (IsMagicTag(a.category) && IsArenaTag(b.category))
	{
		MagicBase* magic = static_cast<MagicBase*>(a.owner);
		if (!magic->GetIsAlive()) return; // 既に何かに当たって消えた魔法は無視する

		if (magic->GetMagicKind() == MagicKind::Rapid) magic->PlayHitEffect(a.pos);
		magic->OnTerminate(); // 魔法は消える
	}
}

// 1フレーム分のColliderをかき集める
std::vector<Collider> CollisionManager::GatherColliders(Player& player, Enemy& enemy, Stage& stage)
{
	std::vector<Collider> colliders;

	// プレイヤー本体
	Collider playerBody;
	playerBody.category = CollisionTag::PlayerBody;
	playerBody.targetMask =
		static_cast<uint32_t>(CollisionTag::EnemyBody) |
		static_cast<uint32_t>(CollisionTag::Arena);
	playerBody.shape = HitBoxShape::Capsule;
	playerBody.pos = player.GetPosition();
	playerBody.pos.y += player.GetCollisonOffset();
	playerBody.dir = player.GetCapsuleDirection();
	playerBody.radius = player.GetRadius();
	playerBody.height = player.GetHeight();
	playerBody.owner = &player;
	colliders.push_back(playerBody);

	// 敵本体
	Collider enemyBody;
	enemyBody.category = CollisionTag::EnemyBody;
	enemyBody.targetMask = static_cast<uint32_t>(CollisionTag::Arena);
	enemyBody.shape = HitBoxShape::Capsule;
	enemyBody.pos = enemy.GetPosition();
	enemyBody.pos.y += enemy.GetCollisionOffset();
	enemyBody.dir = enemy.GetCapsuleDirection();
	enemyBody.radius = enemy.GetRadius();
	enemyBody.height = enemy.GetHeight();
	enemyBody.owner = &enemy;
	colliders.push_back(enemyBody);

	// 移動範囲
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
				static_cast<uint32_t>(CollisionTag::EnemyDefenseMagic);
		}
		else// MagicKind::Rapid
		{
			playerMagic.category = CollisionTag::PlayerRapidMagic;
			playerMagic.targetMask = static_cast<uint32_t>(CollisionTag::EnemyBody)
				| static_cast<uint32_t>(CollisionTag::EnemyDefenseMagic) |
				static_cast<uint32_t>(CollisionTag::Arena);
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
				static_cast<uint32_t>(CollisionTag::PlayerBody) |
				static_cast<uint32_t>(CollisionTag::PlayerDefenseMagic);

		}
		else// MagicKind::Rapid
		{
			enemyMagic.category = CollisionTag::EnemyRapidMagic;
			enemyMagic.targetMask =
				static_cast<uint32_t>(CollisionTag::PlayerBody) |
				static_cast<uint32_t>(CollisionTag::PlayerDefenseMagic) |
				static_cast<uint32_t>(CollisionTag::Arena);
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
		playerGuardCollider.targetMask = static_cast<uint32_t>(CollisionTag::None);
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
		enemyGuardCollider.targetMask = static_cast<uint32_t>(CollisionTag::None);
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




// 通常魔法系の線分上の近い点を返す関数
DirectX::XMFLOAT3 CollisionManager::ComputeHitPosition(const Collider& magic, const Collider& target)
{
	using namespace DirectX;

	// 球の魔法（連射魔法）は、今の位置がそのまま当たった位置になる
	if (magic.shape != HitBoxShape::Capsule) return magic.pos;

	// レーザーの始点と向き（posは真ん中なので半分戻したところが始点になる）
	XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&magic.dir));
	XMVECTOR start = XMVectorSubtract(XMLoadFloat3(&magic.pos), XMVectorScale(dir, magic.height * 0.5));

	// 相手の中心をレーザーの線上に落とす（始点から何メートル先か）
	XMVECTOR toTarget = XMVectorSubtract(XMLoadFloat3(&target.pos), start);
	float t = XMVectorGetX(XMVector3Dot(toTarget, dir));
	t = std::clamp(t, 0.0f, magic.height);

	// 相手の半径分手前に戻す（身体の表面で爆発させるため）
	t = (std::max)(0.0f, t - target.radius);

	XMFLOAT3 hit;
	XMStoreFloat3(&hit, XMVectorAdd(start, XMVectorScale(dir, t)));
	return hit;
}

// 通常魔法（レーザー）をステージにレイキャストして、当たった所にエフェクトを出す
void CollisionManager::CheckMagicVsStage(MagicManager& magicMgr, Stage& stage)
{
	using namespace DirectX;

	for (int i = 0; i < magicMgr.GetMagicCount(); ++i)
	{
		MagicBase* magic = magicMgr.getMagic(i);
		if (!magic->GetIsAlive()) continue;

		XMFLOAT3 startF, dirF, hitPos, hitNormal;
		float distance = magic->GetHeight(); // レーザーの長さ分だけ飛ばす
		float castRadius = 0.1f;

		if (magic->GetMagicKind() == MagicKind::Normal)
		{
			// レーザーの始点（posは真ん中なので半分戻す）
			XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&magic->GetDirection()));
			XMVECTOR start = XMVectorSubtract(XMLoadFloat3(&magic->GetPosition()),
				XMVectorScale(dir, magic->GetHeight() * 0.5f));

			XMStoreFloat3(&startF, start);
			XMStoreFloat3(&dirF, dir);
		}
		else
		{
			// 前のフレームの位置から今の位置まで
			XMVECTOR prev = XMLoadFloat3(&magic->GetPrevPosition());
			XMVECTOR move = XMVectorSubtract(XMLoadFloat3(&magic->GetPosition()), prev);
			distance = XMVectorGetX(XMVector3Length(move));
			if (distance < 0.0001f) continue; // まだ動いてない

			XMStoreFloat3(&startF, prev);
			XMStoreFloat3(&dirF, XMVectorScale(move, 1.0f / distance));
			castRadius = magic->GetRadius(); // 弾の大きさで当てる
		}

		if (Collision::SphereCast(startF, dirF, castRadius, distance,
			stage.GetTriangles(), stage.GetAreas(), hitPos, hitNormal, true))
		{
			magic->PlayHitEffect(hitPos);
			magic->OnTerminate(); // 魔法は消える
		}
	}
}



// このタグはキャラクター本体か？
bool CollisionManager::IsCharacterTag(CollisionTag tag)
{
	return tag == CollisionTag::PlayerBody || tag == CollisionTag::EnemyBody;
}

// このタグはエリアか？
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