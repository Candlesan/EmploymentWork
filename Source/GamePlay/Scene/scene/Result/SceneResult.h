#pragma once
#include "Gameplay/Scene/Manager/Scene.h"
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"

#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

class SceneResult : public Scene
{
public:
	enum class ResultMenu { None, Retry, ReBuid, toTitle };

public:
	SceneResult() {}
	~SceneResult() override {}

	//初期化
	void Initialize() override;

	//終了化
	void Finalize() override;

	//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render() override;

	//GUI描画
	void DrawGUI() override;

	// 1文字を切り抜いて表示する
	void SetDigit(const char* name, int number);

	// クリア装備を表示する
	void ShowEquipped(const char* slotName, MagicSlotIndex slot);
private:
	UIEditor uiEditor;
	UICanvas uiCanvas;

	ResultMenu items[3] = { ResultMenu::Retry, ResultMenu::ReBuid, ResultMenu::toTitle };
	ResultMenu selected = ResultMenu::None;   // フェード後にどこへ行くか


	bool isFadeing = false;
};

