#pragma once
#include <DirectXMath.h>
#include "json.hpp"

class Enemy;

// 発射情報
struct MagicSpawnInfo
{
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f }; // プレイヤーの座標
	float angleY = 0.0f;                               // プレイヤーの向き
	Enemy* targetEnemy = nullptr;                      // ロックオンしてる敵
};

// 魔法を出すための情報
struct MagicInfo
{
	float damage = 100.0f; // ダメージ

	float speed = 20.0f; // 球の飛ぶ速度
	float lifeTimer = 3.0f; // 球が消えるまでの時間

	float hitRadius = 1.0f; // 当たり半径の半径
	float hitHeight = 1.0f; // 当たり判定の高さ（カプセル用）
	float scale = 1.0f; // エフェクトの大きさ

	DirectX::XMFLOAT3 spawnOffset = { 0.0f, 1.0f, 0.0f };

	void SaveBase(nlohmann::json& j) const
	{
		using json = nlohmann::json;

		j["damage"] = damage;
		j["speed"] = speed;
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
		if (j.find("speed") != j.end()) { speed = j["speed"]; }
		if (j.find("lifeTimer") != j.end()) { lifeTimer = j["lifeTimer"]; }
		if (j.find("hitRadius") != j.end()) { hitRadius = j["hitRadius"]; }
		if (j.find("hitHeight") != j.end()) { hitHeight = j["hitHeight"]; }
		if (j.find("scale") != j.end()) { scale = j["scale"]; }
		if (j.find("spawnOffsetX") != j.end()) { spawnOffset.x = j["spawnOffsetX"]; }
		if (j.find("spawnOffsetY") != j.end()) { spawnOffset.y = j["spawnOffsetY"]; }
		if (j.find("spawnOffsetZ") != j.end()) { spawnOffset.z = j["spawnOffsetZ"]; }
	}
};