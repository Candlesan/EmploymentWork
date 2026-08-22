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

// ブレンドモードの定義
enum class BlendMode
{
	Alpha,    // 通常半透明 (Transparency)
	Add,      // 加算合成 (Additive)
};

// 描画方式
enum class RenderType
{
	Particle = 0, // 2Dビルボード
	Mesh = 1, // 3DメッシュVFX
	Trail = 2, // 軌跡
	None = 3
};

struct StateRange
{
	std::string name; // タグ(start/loop/end)
	float start = 0.0f; // 始め
	float end = 0.0f; // 終わり
};

// ノード設定
struct NodeConfig 
{
	std::string name = "New Node";										// ノードの表示名
	RenderType type = RenderType::Particle;								// ノードの種類
	bool visible = true;												// エディターでの表示/非表示フラグ

	// --- 共通パラメータ（時間・生成・移動計算）---
	int emitCount = 1;													// パーティクルの生成数
	float lifeTime = 1.0f;												// 消滅までの寿命
	float delayTime = 0.0f;												// パーティクルを生成するまでの時間
	float duration = 1.0f;												// エフェクト全体の生存時間
	float spawnInterval = 1.0f;											// 次のパーティクルを出すまでの間隔(秒)
	float spawnTimer = 0.0f;											// 毎フレーム進める発生計算用タイマー
	int blendMode = 0;													// 0: Alpha, 1: Add
	bool isLoop = true;													// ループするか

	DirectX::XMFLOAT2 uvScrollSpeed = { 0.0f, 0.0f };					// テクスチャのUVスクロール
	DirectX::XMFLOAT2 noiseUvOffset = { 0.0f, 0.0f };					// ノイズテクスチャのUVスクロール

	// トランスフォーム
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

	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };						// スケール
	DirectX::XMFLOAT3 scaleRange = { 0.0f, 0.0f, 0.0f };						// 大きさのバラつき
	DirectX::XMFLOAT3 scaleVelocity = { 0.0f, 0.0f, 0.0f };				// 拡大速度
	DirectX::XMFLOAT3 scaleVelocityRange = { 0.0f, 0.0f, 0.0f };		// 拡大速度のばらつき
	DirectX::XMFLOAT3 scaleAcceleration = { 0.0f, 0.0f, 0.0f };			// 拡大速度の加速度
	DirectX::XMFLOAT3 scaleAccelerationRange = { 0.0f, 0.0f, 0.0f };	// 拡大速度の加速度のバラつき

	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };				// RGBA

	// --- 描画タイプごとの固有パラメータ ---
	// [Particle用]
	std::string texturePath = "";										// 使うpng画像
	int splitCount[2] = { 1, 1 };										// 画像の分割数（X, Y。デフォルトは4x4）
	int spriteType = 0;													// コマ番号
	bool isBillboard = true;											// ビルボードするか

	// [Mesh用]
	std::string meshPath = "";											// モデルパス
	std::string noisePath = "";											// ノイズテクスチャパス
	float distortionPower = 0.0f;										// 歪みの強さ

	// [Trail用]
	float width = 1.0f;													
	float widthStart = 1.0f;											// 先端
	float widthMiddle = 1.0f;											// 中央
	float widthEnd = 0.0f;												// 末端
};

// 1つのVFXアセット
struct EffectConfig
{
	std::string effectName = "NewEffect";

	std::vector<NodeConfig> nodes;
	std::vector<StateRange> ranges;

	EffectConfig()
	{
		NodeConfig defaultNode;
		defaultNode.name = "Node";
		defaultNode.type = RenderType::Particle;
		nodes.push_back(defaultNode);
	}

	void Save(const std::string& filepath);
	void Load(const std::string& filepath);
};