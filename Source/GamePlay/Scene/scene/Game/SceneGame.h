#pragma once
// ゲームオブジェクト
#include "Gameplay/Scene/Manager/Scene.h"
#include "GamePlay/Scene/scene/Pause/PauseMenu.h"
#include "GamePlay/Scene/scene/Retry/RetryMenu.h"

#include "Gameplay/Object/Camera/DebugCamera/DebugCamera.h"
#include "Gameplay/Object/Camera/GameCamera/GameCamera.h"
#include "GamePlay/Object/Particle/Compute_Particle.h"

// モデル
#include "Gameplay/Object/Stage/Stage.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"

// 当たり判定
#include "GamePlay/Object/Collision/CollisionManager.h"

// UI
#include "GamePlay/Object/Character/Player/PlayerUI.h"
#include "GamePlay/Object/Character/Enemy/EnemyUI.h"

// シェーダー
#include "System/Shader/SceneRenderEnv/SceneRenderEnv.h"

// エディター
#include "System/UI/EffectEditor/EffectEditor.h"
#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <memory>
#include <string>


// ゲームシーン
class SceneGame : public Scene
{
public:
	SceneGame() {};
	~SceneGame() override {};

	// 初期化
	void Initialize() override;

	// 終了化
	void Finalize() override;

	// 更新処理
	void Update(float elapsedTime) override;

	// 描画処理
	void Render() override;

	void RenderCharacterShadowMap();

	// GUI描画
	void DrawGUI() override;

	// 画面比率修正関数
	void OnResize(float width, float height);

private:

	void DrawModelMaterialEditor(const char* label, Character* character);

	void Save(const std::string& path, Character* character);
	void Load(const std::string& path, Character* character);

	// デバック表示のON/OFFを切り替える関数
	void ShowDebugSwitch();
private:
	// 3Dモデル
	std::unique_ptr<Stage> stage;
	std::unique_ptr<Enemy> enemy;

	// カメラ
	std::unique_ptr<GameCamera> gameCamera;
	std::unique_ptr<DebugCamera> debugCamera;

	SceneRenderEnv sceneRender;

	// カメラのモード定義
	enum class CameraMode
	{
		Game,   // 三人称視点
		Debug   // フリーカメラ
	};
	CameraMode cameraMode = CameraMode::Game; // 初期状態はゲーム用
	float cameraRange = 7.0f;
	DirectX::XMFLOAT3 cameraAngle = { 0, 0, 0 };

	PointLight playerLight; // 周りを照らす用の弱い照明
	PointLight enemyLight;
	PointLight playerFaceLight; // キャラクターの顔を照らすライト
	PointLight enemyFaceLight;
	DirectX::XMFLOAT3 playerLightPos = { 1, 2.02f, 0.76f };
	DirectX::XMFLOAT3 enemyLightPos  = { 1, 2.02f, 0.76f };

	// キャラクターの影
	float characterShadowAttenuation = 0.0f;
	float characterShadowBias = 0.000f;
	DirectX::XMFLOAT4X4 characterLightViewProjection{};

	// パーティクル
	std::unique_ptr<Compute_Particle> particleManager;

	// コリジョンマネージャー
	CollisionManager collisonManager;
	bool showDebugSphere = false;

	// エフェクトエディター関係
	std::unique_ptr<EffectEditor> effectEditor;
	std::string effectName;
	bool showEffectEditor = true;

	// UI関係
	UIEditor uiEditor;
	UICanvas uiCanvas;
	PlayerUI playerUI;
	EnemyUI enemyUI;
	bool showUIEdlitor = true;

	UIElement* fade = nullptr;
	bool isFadeing = false;

	PauseMenu pauseMenu;
	bool isPaused = false;

	RetryMenu retryMenu;
	bool playerDead = false;
	bool isRetry = false;
	bool reBuild = false;
	bool toTitle = false;

	// ----- デバック機能 -----
	bool isDebugPaused = false;      // 一時停止中か
	bool stepNextFrame = false; // 1フレームだけ進めるか

	// キーのエッジ検出用（前フレームの押下状態を保持）
	bool wasF5DownLastFrame = false;
	bool wasF6DownLastFrame = false;

	float timeScale = 1.0f; // 1.0が通常、0.1が10倍スロー
	float baseTimeScale = 1.0f;

	// デバックプリミティブの描画のONとOFF
	bool showDebugPrimitive = true;

	// デバック用のカメラや再生速度などの便利機能のON/OFF
	bool showDebugOption = true; 

	bool invertY = false;

	bool showModel = true; // モデルを表示するか
	bool showUI = true; // UIを表示するか
};