#pragma once
#include "Gameplay/Scene/Manager/Scene.h"

#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

class SceneResult : public Scene
{
public:
	SceneResult() {}
	~SceneResult() override {}

	//‰Šú‰»
	void Initialize() override;

	//I—¹‰»
	void Finalize() override;

	//XVˆ—
	void Update(float elapsedTime) override;

	//•`‰æˆ—
	void Render() override;

	//GUI•`‰æ
	void DrawGUI() override;

	// ‰æ–Ê”ä—¦C³ŠÖ”
	void OnResize(float width, float height);

private:
	UIEditor uiEditor;
	UICanvas uiCanvas;
};

