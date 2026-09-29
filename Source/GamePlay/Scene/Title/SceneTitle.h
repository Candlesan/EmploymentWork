#pragma once
#include "GamePlay/Scene/Manager/Scene.h"

#include "System/Renderer/Shader/FrameBuffer/FrameBuffer.h"
#include "System/Renderer/Shader/fullscreen_quad/fullscreen_quad.h"
#include "System/Renderer/Shader/SkyMap/SkyMap.h"
#include "System/Renderer/Shader/bloom/bloom.h"
#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <memory>


class SceneTitle : public Scene
{
public:
	SceneTitle() {}
	~SceneTitle() override {}

	//‰Šú‰»
	void Initialize() override;

	//I—¹‰»
	void Finalize() override {};

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