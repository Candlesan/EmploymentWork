#pragma once
#include "GamePlay/Scene/Manager/Scene.h"

#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <memory>


class SceneTitle : public Scene
{
public:
	enum class GameMenu { None, GameStart, Config, GameEnd };

public:
	SceneTitle() {}
	~SceneTitle() override {}

	//初期化
	void Initialize() override;

	//終了化
	void Finalize() override {};

	//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render() override;

	//GUI描画
	void DrawGUI() override;

	// 画面比率修正関数
	void OnResize(float width, float height);
private:
	UIEditor uiEditor;
	UICanvas uiCanvas;

	GameMenu items[3] = { GameMenu::GameStart, GameMenu::Config, GameMenu::GameEnd };
	GameMenu selected = GameMenu::None;   // フェード後にどこへ行くか

	bool isFadeing = false;
};