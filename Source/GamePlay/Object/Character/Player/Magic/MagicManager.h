#pragma once
#include "GamePlay/Object/Character/Player/Magic/MagicBase.h"
#include <vector>
#include <set>


//　マジックマネージャー
class MagicManager
{
public:
	MagicManager();
	~MagicManager();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc);

	//デバックプリミティブ描画
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	//弾丸登録
	void Register(MagicBase* projectile);

	//弾丸全削除
	void Clear();

	//弾丸数取得
	int GetMagicCount() const { return static_cast<int>(projectiles.size()); }

	//弾丸取得
	MagicBase* getMagic(int index) { return projectiles.at(index); }

	//弾丸削除
	void Remove(MagicBase* projectile);

private:
	std::vector<MagicBase*> projectiles;
	std::set<MagicBase*> removes;

};