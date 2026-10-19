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
	Ring = 4, // リング
	None = 3,
};

// Lerpの速度
enum class SpeedType
{
	Slow3 = 0,	// 低速3
	Slow2,		// 低速2
	Slow1,		// 低速1
	Linear,		// 等速
	Fast1,		// 高速1
	Fast2,		// 高速2
	Fast3,		// 高速3
};

// 拡大縮小のモード
enum class ScaleMode
{
	Fixed,			// 固定
	UniformEasing,	// 単一イージング
};

// 色のモード
enum class ColorMode
{
	Fixed = 0,	// 固定色
	Random,		// ランダム(最小～最大の間で生成時に決定)
	Easing		// イージング(始点色から終点色へ寿命で変化)
};

// ノード設定
struct NodeConfig 
{
	int id = 0;															// ノードを区別する番号
	int parentId = -1;													// 親ノードのid(-1 = エフェクト直下)

	std::string name = "New Node";										// ノードの表示名
	RenderType type = RenderType::Particle;								// ノードの種類
	bool visible = true;												// エディターでの表示/非表示フラグ

	// --- 共通パラメータ（時間・生成・移動計算）---
	int emitCount = 1;													// パーティクルの生成数
	float lifeTime = 1.0f;												// 消滅までの寿命
	float delayTime = 0.0f;												// パーティクルを生成するまでの時間
	float duration = 1.0f;												// エフェクト全体の生存時間
	float spawnInterval = 1.0f;											// 次のパーティクルを出すまでの間隔(秒)
	int blendMode = 0;													// 0: Alpha, 1: Add
	bool isLoop = true;													// ループするか

	// --- フェードイン ---
	bool IsFadeIn = false;												// フェードインするか
	float fadeInTime = 0.0f;											// フェードインが終わる時間
	int fadeInStartSpeed = static_cast<int>(SpeedType::Linear);			// 生成されてからの速度
	int fadeInEndSpeed = static_cast<int>(SpeedType::Linear);			// 生存時間が終わるにつれての速度

	// --- フェードアウト ---
	bool IsFadeOut = false;												// フェードインするか
	float fadeOutTime = 0.0f;											// フェードインが終わる時間
	int fadeOutStartSpeed = static_cast<int>(SpeedType::Linear);		// 生成されてからの速度
	int fadeOutEndSpeed = static_cast<int>(SpeedType::Linear);			// 生存時間が終わるにつれての速度

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
	DirectX::XMFLOAT3 renderScale = { 1.0f, 1.0f, 1.0f };				// スケール(描画)
	DirectX::XMFLOAT3 scaleRange = { 0.0f, 0.0f, 0.0f };				// 大きさのバラつき
	DirectX::XMFLOAT3 scaleVelocity = { 0.0f, 0.0f, 0.0f };				// 拡大速度
	DirectX::XMFLOAT3 scaleVelocityRange = { 0.0f, 0.0f, 0.0f };		// 拡大速度のばらつき
	DirectX::XMFLOAT3 scaleAcceleration = { 0.0f, 0.0f, 0.0f };			// 拡大速度の加速度
	DirectX::XMFLOAT3 scaleAccelerationRange = { 0.0f, 0.0f, 0.0f };	// 拡大速度の加速度のバラつき

	// 単一イージング（拡大縮小）
	ScaleMode scaleMode = ScaleMode::Fixed;
	float scaleUniformStart = 1.0f;										// 始点：中心
	float scaleUniformStartRandom = 0.0f;								// 始点：振れ幅

	float scaleUniformEnd = 1.0f;										// 終点：中心
	float scaleUniformEndRandom = 0.0f;									// 終点：振れ幅

	int scaleUniformStartSpeed = static_cast<int>(SpeedType::Linear);	// 生成されてからの速度
	int scaleUniformEndSpeed = static_cast<int>(SpeedType::Linear);		// 生存時間が終わるにつれての速度

	// --- カラー --- 
	ColorMode colorMode = ColorMode::Fixed;
	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };				// 基本色
	DirectX::XMFLOAT4 colorMin = { 1.0f, 1.0f, 1.0f, 1.0f };			// カラーの最小値（ランダム用）
	DirectX::XMFLOAT4 colorMax = { 1.0f, 1.0f, 1.0f, 1.0f };			// カラーの最大値（ランダム用）
	DirectX::XMFLOAT4 colorStartMin = { 1.0f, 1.0f, 1.0f, 1.0f };		// 始点のカラー最小値
	DirectX::XMFLOAT4 colorStartMax = { 1.0f, 1.0f, 1.0f, 1.0f };		// 始点のカラー最小値
	DirectX::XMFLOAT4 colorEndMin = { 1.0f, 1.0f, 1.0f, 1.0f };			// 始点のカラー最小値
	DirectX::XMFLOAT4 colorEndMax = { 1.0f, 1.0f, 1.0f, 1.0f };			// 始点のカラー最小値
	int colorStartSpeed = static_cast<int>(SpeedType::Linear);			// 生成されてからの速度
	int colorEndSpeed = static_cast<int>(SpeedType::Linear);			// 生存時間が終わるにつれての速度

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
	float trailLength = 0.3f;											// 帯の長さ(秒)

	// [Ring用]
	int ringDivision = 32;												// 分割数(何角形で円をつくるか)
	DirectX::XMFLOAT2 ringOuter = { 1.0f, 0.0f };						// 外輪 (X = 半径, Y = 高さ)
	DirectX::XMFLOAT2 ringInner = { 0.5f, 0.0f };						// 内輪 (X = 半径, Y = 高さ)
	float ringStartAngle = 0.0f;										// 開始角度(度)
	float ringEndAngle = 360.0f;										// 終了角度(度)。180で半円
	float ringCenterRatio = 0.5f;										// 中心の位置（0 = 外輪, 1 = 内輪）
	DirectX::XMFLOAT4 ringOuterColor = { 1, 1, 1, 1 };
	DirectX::XMFLOAT4 ringCenterColor = { 1, 1, 1, 1 };
	DirectX::XMFLOAT4 ringInnerColor = { 1, 1, 1, 1 };
};

// 1つのVFXアセット
struct EffectConfig
{
	std::string effectName = "NewEffect";

	std::vector<NodeConfig> nodes;

	EffectConfig()
	{
		NodeConfig defaultNode;
		defaultNode.id = 1;
		defaultNode.name = "Node";
		defaultNode.type = RenderType::Particle;
		nodes.push_back(defaultNode);
	}

	// 新しいノード用に、まだ使われていないidを返す（今ある最大値の＋１）
	int GenerateNodeId() const;

	// idから配列の何番目かを探す
	int FindNodeIndex(int id) const;

	// candidateId のノードが ancestorId の子孫か（親をたどって ancestorId に着いたら子孫）
	bool IsDescendant(int candidateId, int ancestorId) const;

	void Save(const std::string& filepath);
	void Load(const std::string& filepath);
};