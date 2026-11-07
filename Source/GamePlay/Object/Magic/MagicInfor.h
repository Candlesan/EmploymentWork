#pragma once
#include <DirectXMath.h>
#include "json.hpp"

// 前方宣言
class Character;

// XMFLOAT3 を JSON に保存・読み込みする
inline void SaveFloat3(nlohmann::json& j, const char* key, const DirectX::XMFLOAT3& v)
{
	j[key] = { v.x, v.y, v.z };
}
inline void LoadFloat3(const nlohmann::json& j, const char* key, DirectX::XMFLOAT3& v)
{
	if (j.find(key) != j.end()) v = { j[key][0], j[key][1], j[key][2] };
}

// 発射情報
struct MagicSpawnInfo
{
	Character* target = nullptr;					    // 射撃対象
	Character* owner = nullptr;							// 射撃者
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };  // プレイヤーの座標
	DirectX::XMFLOAT3 targetPos = { 0.0f, 0.0f, 0.0f }; // 狙ってる位置
	DirectX::XMFLOAT3 direction = { 0.0f, 0.0f, 0.0f }; // 撃つ方向
	float holdTime = 1.0f;								// 長押しした時間
	float angleY = 0.0f;                                // プレイヤーの向き
};

// 魔法を出すための情報
struct MagicInfo
{
	float damage = 1.0f; // ダメージ
	float guardDamage = 1.0f; // 防御魔法へのダメージ

	float magicCost = 1.0f;   // 魔力の消費

	float speed = 100.0f; // 球の飛ぶ速度
	float interval = 1.0f; // 次に撃てるまでの時間

	float lifeTimer = 1.0f; // 球が消えるまでの時間

	float hitRadius = 1.0f; // 当たり半径の半径
	float hitHeight = 1.0f; // 当たり判定の高さ（カプセル用）
	float scale = 1.0f; // エフェクトの大きさ

	DirectX::XMFLOAT3 spawnOffset = { 0.0f, 1.0f, 0.0f }; // 連射魔法の発射位置

	void SaveBase(nlohmann::json& j) const
	{
		using json = nlohmann::json;

		j["damage"] = damage;
		j["guardDamage"] = guardDamage;
		j["magicCost"] = magicCost;
		j["speed"] = speed;
		j["interval"] = interval;
		j["lifeTimer"] = lifeTimer;
		j["hitRadius"] = hitRadius;
		j["hitHeight"] = hitHeight;
		j["scale"] = scale;
		j["spawnOffsetX"] = spawnOffset.x;
		j["spawnOffsetY"] = spawnOffset.y;
		j["spawnOffsetZ"] = spawnOffset.z;
	}

	void LoadBase(const nlohmann::json& j)
	{
		if (j.find("damage") != j.end()) { damage = j["damage"]; }
		if (j.find("guardDamage") != j.end()) { guardDamage = j["guardDamage"]; }
		if (j.find("magicCost") != j.end()) { magicCost = j["magicCost"]; }
		if (j.find("speed") != j.end()) { speed = j["speed"]; }
		if (j.find("interval") != j.end()) { interval = j["interval"]; }
		if (j.find("lifeTimer") != j.end()) { lifeTimer = j["lifeTimer"]; }
		if (j.find("hitRadius") != j.end()) { hitRadius = j["hitRadius"]; }
		if (j.find("hitHeight") != j.end()) { hitHeight = j["hitHeight"]; }
		if (j.find("scale") != j.end()) { scale = j["scale"]; }
		if (j.find("spawnOffsetX") != j.end()) { spawnOffset.x = j["spawnOffsetX"]; }
		if (j.find("spawnOffsetY") != j.end()) { spawnOffset.y = j["spawnOffsetY"]; }
		if (j.find("spawnOffsetZ") != j.end()) { spawnOffset.z = j["spawnOffsetZ"]; }
	}
};