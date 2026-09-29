#pragma once
#include "GamePlay/Scene/Manager/Scene.h"

#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <thread>

class SceneLoad : public Scene
{
public:
	SceneLoad(Scene* nextScene) : nextScene(nextScene) {}
	~SceneLoad() override {}

	//初期化
	void Initialize() override;

	//終了化
	void Finalize() override;

	//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render() override;

	// シャドウマップ描画
	void RenderShadowMap();

	//GUI描画
	void DrawGUI() override;

	// 画面比率修正関数
	void OnResize(float width, float height);

private:
	// ローディングスレッド
	static void LoadingThread(SceneLoad* scene);

private:
	UIEditor uiEditor;
	UICanvas uiCanvas;

	Scene* nextScene = nullptr;
	std::thread* thread = nullptr;
};