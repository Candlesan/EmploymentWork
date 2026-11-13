#pragma once
#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include "GamePlay/Scene/Manager/Scene.h"

class SceneBuild : public Scene
{
public:
	SceneBuild() {}
	~SceneBuild() override {}

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

	// 装備中の魔法に Equipped の印を付ける
	void RefreshEquippedMarks();
private:
	UIEditor uiEditor;
	UICanvas uiCanvas;

	bool isFadeing = false;
};