#pragma once
#include "Scene.h"
#include "Gameplay/Object/Camera/DebugCamera/DebugCamera.h"
#include "Gameplay/Object/Camera/GameCamera/GameCamera.h"
#include "GamePlay/Object/Particle/Compute_Particle.h"
#include <memory>

// モデル
#include "Gameplay/Object/Stage/Stage.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Collision/CollisionManager.h"

#include "System/Renderer/Shader/FrameBuffer/FrameBuffer.h"
#include "System/Renderer/Shader/fullscreen_quad/fullscreen_quad.h"
#include "System/Renderer/Shader/SkyMap/SkyMap.h"
#include "System/Renderer/Shader/bloom/bloom.h"
#include "System/UI/EffectEditor/EffectEditor.h"


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
	void RenderShadowMap();

	// GUI描画
	void DrawGUI() override;

	void DrawModelMaterialEditor(const char* label, Character* character);

	void Save(const std::string& path, Character* character);
	void Load(const std::string& path, Character* character);

	// 画面比率修正関数
	void OnResize(float width, float height);
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

	float debugOffset = 0.5;

	// デバック機能
	bool isPaused = false;      // 一時停止中か
	bool stepNextFrame = false; // 1フレームだけ進めるか
	// キーのエッジ検出用（前フレームの押下状態を保持）
	bool wasF5DownLastFrame = false;
	bool wasF6DownLastFrame = false;

	float timeScale = 1.0f; // 1.0が通常、0.1が10倍スロー
	float baseTimeScale = 1.0f;

	std::unique_ptr<SkyMap> skyMap; // スカイマップ

	// PBR調整用
	float pbrMetalness = 0.6f;
	float pbrRoughness = 1.0f;

	// シャドウマップ調整用
	float shadowAttenuation = 0.125f;
	float shadowBias = 0.002f;
	DirectX::XMFLOAT4X4 lightViewProjection;

	// ブルーム調整用
	std::unique_ptr<bloom> bloomer;
	std::unique_ptr<framebuffer> framebuffers[8];
	std::unique_ptr<fullscreen_quad> bit_block_transfer;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];
	float bloom_extraction = 1.0f;
	float bloom_intensity = 0.29f;

	// パーティクル
	std::unique_ptr<Compute_Particle> particleManager;

	// コリジョンマネージャー
	CollisionManager collisonManager;
	bool showDebugSphere = false;

	std::unique_ptr<EffectEditor> effectEditor;
	bool showVfxEditor = false;
};