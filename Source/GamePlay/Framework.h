#pragma once
#include "System/Core/HighResolutionTimer.h"

#include "Gameplay/Scene/Manager/Scene.h"

#include <windows.h>
#include <memory>

class Framework
{
public:
	Framework(HWND hWnd);
	~Framework();

private:
	void Update(float elapsedTime);
	void Render(float elapsedTime);

	void CalculateFrameStats();
	void ToggleFullscreen();
	void HandleResize(float w, float h);
public:
	int Run();
	LRESULT CALLBACK HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	const HWND				hWnd;
	HighResolutionTimer		timer;
	std::unique_ptr<Scene>	scene;
	RECT windowedRect = {};

	bool isFullscreen = false;

#ifdef _DEBUG
	bool GUIDrawSwitch = true; // デバックは起動時にGUIを表示する
#else
	bool GUIDrawSwitch = false; // リリースは起動時にGUIを非表示にする
#endif // DEBUG

	bool isF1PressedLastFrame = false;
};