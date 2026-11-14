#pragma once
#include "GamePlay/Scene/Manager/Scene.h"

#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <memory>


class RetryMenu
{
public:
	enum class Retry { None, Retry, ReBuild, ToTitle };

public:
	RetryMenu() {}
	~RetryMenu() {}

	//‰Šú‰»
	void Initialize();

	//XVˆ—
	Retry Update(float elapsedTime); // ‘I‚Î‚ê‚½€–Ú‚ğ•Ô‚·

	//•`‰æˆ—
	void Render();

private:
	UICanvas uiCanvas;
	Retry items[3] = { Retry::Retry, Retry::ReBuild, Retry::ToTitle };
};