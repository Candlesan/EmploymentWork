#pragma once
#include "System/UI/UIEditor/UICanvas.h"

struct CharacterStatus;

class PlayerUI
{
public:
	// g‚¤UI‚ğ‚±‚±‚Åæ‚É’T‚µ‚Ä‚¨‚­
	void Initialize(UICanvas& uiCanvas);

	// UI‚ğXV‚·‚é
	void Update(const CharacterStatus& status);

private:
	UIElement* hpGauge = nullptr;
	UIElement* mpGauge = nullptr;
	UIElement* guardGauge = nullptr;
	UIElement* heatGauge = nullptr;
};
