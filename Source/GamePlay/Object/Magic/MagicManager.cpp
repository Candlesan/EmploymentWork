#include "MagicManager.h"
#include "GamePlay/Object/Magic/MagicProjectile.h"
#include "GamePlay/Object/Magic/MagicMove/StraightMagic/StraightMove.h"
#include "GamePlay/Object/Character/Character.h"

#include <algorithm>

// コンストラクタ
MagicManager::MagicManager()
{
}

// デストラクタ
MagicManager::~MagicManager()
{
	Clear();
}

// 更新処理
void MagicManager::Update(float elapsedTime)
{
	//更新処理
	for (MagicBase* projectile : projectiles)
	{
		projectile->Update(elapsedTime);
	}

	//破棄処理
	//※projectilesの範囲for文中でerase()すると不具合が発生してしまうため、
	//更新処理が終わった後に破棄リストに積まれたオブジェクトを削除する。
	for (MagicBase* projectile : removes)
	{
		//std;;vrctorから要素を削除する場合はイテレーターで削除しなければならない
		std::vector<MagicBase*>::iterator it = std::find(projectiles.begin(),
			projectiles.end(), projectile);
		if (it != projectiles.end())
		{
			projectiles.erase(it);

			//弾丸の破棄処理
			delete projectile;
		}

	}
	//破棄リストをクリア
	removes.clear();
}

// 描画処理
void MagicManager::Render(const RenderContext& rc)
{
	for (MagicBase* projectile : projectiles)
	{
		projectile->Render(rc);
	}
}

// デバックプリミティブ描画
void MagicManager::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	for (MagicBase* projectile : projectiles)
	{
		projectile->RenderDebugPrimitive(renderer);
	}
}

// 弾丸登録
void MagicManager::Register(MagicBase* projectile)
{
	projectiles.emplace_back(projectile);
}

// 弾を消去する
void MagicManager::Clear()
{
	removes.clear();

	for (MagicBase* projectile : projectiles)
	{
		delete projectile;
	}
	projectiles.clear();
}

//弾丸削除
void MagicManager::Remove(MagicBase* projectile)
{
	//破棄リストに追加
	removes.insert(projectile);
}

// 迎撃範囲内にある魔法を近い順で返す関数
std::vector<MagicBase*> MagicManager::FindNearProjectiles(
	Character* owner,
	const DirectX::XMFLOAT3& centerPos,
	float searchRadius)
{
	using namespace DirectX;
	std::vector<MagicBase*> target;

	if (owner == nullptr) return target;

	// プレイヤー位置を取得する。※プレイヤー位置はfor分の中で変わらないから一回だけ取得する
	XMVECTOR center = XMLoadFloat3(&centerPos);
	float searchRadiusSq = searchRadius * searchRadius; // 事前に半径の2乗を計算する

	// 範囲内に入った球を取得する
	for (MagicBase* p : projectiles) {
		if (p->GetOwner() == owner || !p->GetIsAlive())
			continue; // 弾が生きて無かったら再捜査

		// 距離を計算する
		XMVECTOR magicPos = XMLoadFloat3(&p->GetPosition());

		// プレイヤー位置と魔法の位置の差分を求める
		XMVECTOR diff = XMVectorSubtract(magicPos, center);

		// 長さの2乗を出す
		float lengthSq = XMVectorGetX(XMVector3LengthSq(diff));
		
		if (lengthSq <= searchRadiusSq)
		{
			target.push_back(p);
		}
	}

	// 魔法が近い順でソートする
	std::sort(target.begin(), target.end(),
		[&centerPos](MagicBase* a, MagicBase* b)
		{
			// 迎撃範囲の中心位置を取得する
			XMVECTOR center = XMLoadFloat3(&centerPos);

			// 中心位置からaの距離の2乗を求める
			XMVECTOR posA = XMLoadFloat3(&a->GetPosition());
			XMVECTOR diffA = XMVectorSubtract(posA, center);
			float distSqA = XMVectorGetX(XMVector3LengthSq(diffA));

			// 中心位置からbの距離の2乗を求める
			XMVECTOR posB = XMLoadFloat3(&b->GetPosition());
			XMVECTOR diffB = XMVectorSubtract(posB, center);
			float distSqB = XMVectorGetX(XMVector3LengthSq(diffB));

			// aの方が近ければtrueを返す
			return distSqA < distSqB;
		});

	return target;
}