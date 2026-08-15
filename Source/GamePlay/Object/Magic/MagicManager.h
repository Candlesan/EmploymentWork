#pragma once
#include "GamePlay/Object/Magic/MagicBase.h"
#include <vector>
#include <set>

// 前方宣言
class Character;

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

	// 迎撃範囲内にある魔法を近い順で返す関数
	std::vector<MagicBase*> FindNearProjectiles(
		Character* owner,					// 所有者
		const DirectX::XMFLOAT3& centerPos,	// 迎撃判定中心位置
		float searchRadius);				// 迎撃判定範囲(半径)

	// 現在リストに入っている魔法を取得する
	const std::vector<MagicBase*>& GetProjectiles() const { return projectiles; }
private:
	std::vector<MagicBase*> projectiles;
	std::set<MagicBase*> removes;

};