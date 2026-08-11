#pragma once
#include "GamePlay/Object/Collision/Collision.h"

// 前方宣言
class Character;
class Player;
class Enemy;
class NormalMagic;
class RapidMagic;
class DefenseMagic;
class Stage;

// 当たり判定の属性
enum class CollisitionTag : uint32_t
{
	None				= 0,
	PlayerBody			= 1 << 0, // プレイヤー本体      
	EnemyBody			= 1 << 1, // 敵本体       
	PlayerNomalMagic	= 1 << 2, // プレイヤーの通常攻撃魔法
	PlayerRapidMagic	= 1 << 3, // プレイヤーの連射魔法
	PlayerDefenseMagic	= 1 << 4, // プレイヤーの防御魔法
	EnemyNormalMagic	= 1 << 5, // 敵の通常攻撃魔法
	EnemyRapidMagic		= 1 << 6, // 敵の連射魔法
	EnemyDefenseMagic		= 1 << 7, // 敵の防御魔法
	Stage				= 1 << 8, // ステージ
};

// 当たり判定1つ分のデータ
struct Collider
{
	CollisitionTag category; // 自分の種別
	uint32_t targetMask; // 当たりたい相手

	HitBoxShape shape; // 当たり判定の形状
	DirectX::XMFLOAT3 pos; // 位置
	DirectX::XMFLOAT3 dir; // カプセルの方向
	float radius; // 半径
	float height; // 高さ

	void* owner; // この判定を持っているオブジェクト
};

class CollisionManager {
public:
	// 更新処理
	void Update(Player& player, Enemy& enemy);

private:
	// 当たった時の処理
	void ResolveCollision(const Collider& a, const Collider&b, const DirectX::XMFLOAT3& outPosA, const DirectX::XMFLOAT3& outPosB);

	// ヘルパー関数
	bool IsMagicTag(CollisitionTag tag);
	bool IsCharacterTag(CollisitionTag tag);
	bool IsDefenseTag(CollisitionTag tag);
private:

	// 1フレーム分のColliderをかき集める
	std::vector<Collider> GatherColliders(Player& player, Enemy& enemy);

	// 集めたColliderを総当たりでチェックする
	void CheckCollisions(std::vector<Collider>& colliders);
};
