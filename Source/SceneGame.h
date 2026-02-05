#pragma once
#include "Scene.h"
#include "FreeCameraController.h"
#include <memory>

// モデル
#include "Stage.h"

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

	// GUI描画
	void DrawGUI() override;

private:
	std::unique_ptr<Stage> stage;
	std::unique_ptr<FreeCameraController> cameraController;
	LightManager lightManager;
};

