#pragma once
// ゲームオブジェクト
#include "Gameplay/Scene/Manager/Scene.h"
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
#include "System/Shader/FrameBuffer/FrameBuffer.h"
#include "System/Shader/fullscreen_quad/fullscreen_quad.h"
#include "System/Shader/SkyMap/SkyMap.h"
#include "System/Shader/bloom/bloom.h"

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

	// シャドウマップ描画
	void RenderStageShadowMap();
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

	DirectX::XMFLOAT3 CalcLightDirection() const;
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
	float cameraRange = 7.0f;
	DirectX::XMFLOAT3 cameraAngle = { 0, 0, 0 };

	LightManager lightManager;
	float lightYaw = 0.0f;   // 方位角（度）  0〜360
	float lightPitch = 40.0;  // 仰角（度）   0=地平線, 90=真上

	std::unique_ptr<SkyMap> skyMap; // スカイマップ

	// PBR調整用
	float pbrMetalness = 0.0f;
	float pbrRoughness = 0.0f;
	float ambientIntensity = 0.5;

	// シャドウマップ調整用
	float stageShadowAttenuation = 0.0f;
	float stageShadowBias = 0.006f;
	float characterShadowAttenuation = 0.0f;
	float characterShadowBias = 0.000f;
	DirectX::XMFLOAT4X4 stageLightViewProjection;
	DirectX::XMFLOAT4X4 characterLightViewProjection;
	float stageRadius = 0.0f;
	bool stageShadowRendered = false;

	// ブルーム調整用
	std::unique_ptr<bloom> bloomer;
	std::unique_ptr<framebuffer> framebuffers[8];
	std::unique_ptr<fullscreen_quad> bit_block_transfer;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];
	float bloom_extraction = 0.7f;
	float bloom_intensity = 0.01f;

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
	

	// ----- デバック機能 -----
	bool isPaused = false;      // 一時停止中か
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

	UIElement* fade = nullptr;
	bool isFadeing = false;
};