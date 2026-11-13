#pragma once
#include "GamePlay/Scene/Manager/Scene.h"

#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <memory>


class PauseMenu
{
public:
	enum class Result { None, Resume, Config, ToTitle };

public:
	PauseMenu() {}
	~PauseMenu() {}

	//‰Šú‰»
	void Initialize();

	//XVˆ—
	Result Update(float elapsedTime); // ‘I‚Î‚ê‚½€–Ú‚ğ•Ô‚·

	//•`‰æˆ—
	void Render();

private:
	UICanvas uiCanvas;
	Result items[3] = { Result::Resume, Result::Config, Result::ToTitle };
};