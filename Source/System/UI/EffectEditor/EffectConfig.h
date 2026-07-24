#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "json.hpp"

// ノードの種類
enum class EffectNodeType
{
	Particle,
	Mesh,
	Trail,
};

// パーティクル構造体
struct ParticleEffectConfig
{
	std::string texturePath = "";										// 使うpng画像
	int spriteType = 0;													// コマ番号
	int splitCount[2] = { 1, 1 };										// 画像の分割数（X, Y。デフォルトは4x4）
	int emitCount = 1;													// パーティクルの生成数
	float lifeTime = 1.0f;												// 消滅までの寿命
	float delayTime = 0.0f;												// パーティクルを生成するまでの時間

	bool isLoop = true;													// ループするか
	bool isBillboard = true;											// ビルボードするか
	float spawnInterval = 1.0f;											// 次のパーティクルを出すまでの間隔(秒)
	float spawnTimer = 0.0f;											// 次の発生までのタイマー
	float duration = 1.0f;												// エフェクト全体の生存時間
	DirectX::XMFLOAT2 uvScrollSpeed = { 0.0f, 0.0f };					// パーティクルのUVスクロール

	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };					// 位置
	DirectX::XMFLOAT3 positionRange = { 0.0f, 0.0f, 0.0f };				// 発生位置のバラつき
	DirectX::XMFLOAT3 positionVelocity = { 0.0f, 0.0f, 0.0f };			// 速度
	DirectX::XMFLOAT3 positionVelocityRange = { 0.0f, 0.0f, 0.0f };		// 飛び散る方向のバラつき
	DirectX::XMFLOAT3 positionAcceleration = { 0.0f, 0.0f, 0.0f };		// 位置の加速度
	DirectX::XMFLOAT3 positionAccelerationRange = { 0.0f, 0.0f, 0.0f };	// 位置の加速度のばらつき

	DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };					// 回転
	DirectX::XMFLOAT3 rotationRange = { 0.0f, 0.0f, 0.0f };				// 初期回転のバラつき
	DirectX::XMFLOAT3 rotationVelocity = { 0.0f, 0.0f, 0.0f };			// 角速度（回転スピード）
	DirectX::XMFLOAT3 rotationVelocityRange = { 0.0f, 0.0f, 0.0f };		// 角速度のばらつき
	DirectX::XMFLOAT3 rotationAcceleration = { 0.0f, 0.0f, 0.0f };		// 回転の加速度
	DirectX::XMFLOAT3 rotationAccelerationRange = { 0.0f, 0.0f, 0.0f }; // 回転の加速度のバラつき

	DirectX::XMFLOAT2 scale = { 1.0f, 1.0f };							// スケール
	DirectX::XMFLOAT2 scaleRange = { 0.0f, 0.0f };						// 大きさのバラつき
	DirectX::XMFLOAT3 scaleVelocity = { 0.0f, 0.0f, 0.0f };				// 拡大速度
	DirectX::XMFLOAT3 scaleVelocityRange = { 0.0f, 0.0f, 0.0f };		// 拡大速度のばらつき
	DirectX::XMFLOAT3 scaleAcceleration = { 0.0f, 0.0f, 0.0f };			// 拡大速度の加速度
	DirectX::XMFLOAT3 scaleAccelerationRange = { 0.0f, 0.0f, 0.0f };	// 拡大速度の加速度のバラつき

	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };			// RGBA
};

// メッシュVFX構造体
struct MeshVfxConfig
{
	std::string meshPath = "";									// 使うモデル
	bool useMesh = false;										// メッシュを使うか
	DirectX::XMFLOAT2 scale = { 1.0f, 1.0f };					// スケール
	DirectX::XMFLOAT3 rotationSpeed = { 0.0f, 100.0f, 0.0f };	// 自転速度
	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };		// RGBA
	float lifeTime = 1.0f;										// 寿命
	float delayTime = 0.0f;										// パーティクルを生成するまでの時間
};

// 軌跡構造体
struct TrailConfig
{
	std::string texturePath = "";
	float width = 1.0f;

	float widthStart  = 1.0f; // 先端
	float widthMiddle = 1.0f; // 中央
	float widthEnd    = 0.0f; // 末端


	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float lifeTime = 1.0f;
	float delayTime = 0.0f;
};

// ノード1つのデータ
struct EffectNode
{
	std::string name = "New Node";					// ノードの表示名
	EffectNodeType type = EffectNodeType::Particle;	// ノードの種類
	bool visible = true;							// エディターでの表示/非表示フラグ

	ParticleEffectConfig particle;					// パーティクル設定
	MeshVfxConfig meshVfx;							// メッシュVFX設定
	TrailConfig trail;
};

// 1つのVFXアセット
struct EffectConfig
{
	std::string effectName = "NewEffect";

	std::vector<EffectNode> nodes;

	EffectConfig()
	{
		EffectNode defaultNode;
		defaultNode.name = "Node";
		defaultNode.type = EffectNodeType::Particle;
		nodes.push_back(defaultNode);
	}

	void Save(const std::string& filepath);
	void Load(const std::string& filepath);
};