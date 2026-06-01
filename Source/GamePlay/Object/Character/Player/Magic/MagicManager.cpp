#include "MagicManager.h"

//コンストラクタ
MagicManager::MagicManager()
{
}

//デストラクタ
MagicManager::~MagicManager()
{
	Clear();
}

//更新処理
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
		}

		//弾丸の破棄処理
		delete projectile;
	}
	//破棄リストをクリア
	removes.clear();
}

//描画処理
void MagicManager::Render(const RenderContext& rc)
{
	for (MagicBase* projectile : projectiles)
	{
		projectile->Render(rc);
	}
}

//デバックプリミティブ描画
void MagicManager::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	for (MagicBase* projectile : projectiles)
	{
		projectile->RenderDebugPrimitive(renderer);
	}
}

//弾丸登録
void MagicManager::Register(MagicBase* projectile)
{
	projectiles.emplace_back(projectile);
}

void MagicManager::Clear()
{
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