#pragma once
#include "Scene.h"
#include "Gameplay/Object/Camera/DebugCamera/DebugCamera.h"
#include "Gameplay/Object/Camera/GameCamera/GameCamera.h"
#include <memory>

// モデル
#include "Gameplay/Object/Stage/Stage.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"

#include "System/Renderer/Shader/SkyMap/SkyMap.h"
#include "System/UI/AnimationSequence.h"

// ゲームシーン
class SceneGame : public Scene
{
public:
	SceneGame() {};
	~SceneGame() override {};

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override {};

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render() override;

	// シャドウマップ描画
	void RenderShadowMap();

	// GUI描画
	void DrawGUI() override;

	// プレイヤーと敵の当たり判定
	void CollisonPlayervsEnemy();

	// 武器と敵の当たり判定
	void CollisionPlayerWeaponVsEnemy();

	// 敵の武器とプレイヤーの当たり判定
	void CollisionEnemyWeaponVsPlayer();

private:
	// 3Dモデル
	std::unique_ptr<Stage> stage;
	std::unique_ptr<Enemy> enemy;

	// カメラ
	std::unique_ptr<GameCamera> gameCamera;
	std::unique_ptr<DebugCamera> debugCamera;

	// カメラのモード定義
	enum class CameraMode 
	{
		Game,   // 三人称視点
		Debug   // フリーカメラ
	};
	CameraMode cameraMode = CameraMode::Game; // 初期状態はゲーム用
	float cameraRange = 5.0f;
	DirectX::XMFLOAT3 cameraAngle = { 0, 0, 0 };

	LightManager lightManager;

	float debugOffset = 0.5;

	std::unique_ptr<SkyMap> skyMap; // スカイマップ

	// PBR調整用
	float pbrMetalness = 0.6f;
	float pbrRoughness = 1.0f;

	// シャドウマップ調整用
	float shadowAttenuation = 0.125f;
	float shadowBias = 0.002f;
	DirectX::XMFLOAT4X4 lightViewProjection;

	// シーケンサー関連
	int currentFrame = 0;
	bool sequencerExpanded = true;
	int selectedEntry = -1;
	int firstFrame = 0;
};

