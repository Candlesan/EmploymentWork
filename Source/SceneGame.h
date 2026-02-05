#pragma once
#include "Scene.h"
#include "FreeCameraController.h"
#include <memory>

// モデル
#include "Stage.h"
#include "Player.h"
#include "Enemy.h"

// シェーダー
#include "framebuffer.h"
#include "fullscreen_quad.h"
#include "bloom.h"

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

private:
	std::unique_ptr<Stage> stage;
	std::unique_ptr<FreeCameraController> cameraController;
	LightManager lightManager;
	std::unique_ptr<Player> player;
	std::unique_ptr<Enemy>  enemy;

	// PBR調整用
	float pbrMetalness = -1.0f;
	float pbrRoughness = 1.0f;

	// シャドウマップ調整用
	DirectX::XMFLOAT4X4 lightViewProjection;
	float shadowAttenuation = 0.34;
	float shadowBias = 0.002f;

	// 川瀬式ブルーム調整用
	std::unique_ptr<bloom> bloomer;
	std::unique_ptr<framebuffer> framebuffers[8];
	std::unique_ptr<fullscreen_quad> bit_block_transfer;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[8];
	float bloom_extraction = 1.08f;
	float bloom_intensity = 0.97f;
};