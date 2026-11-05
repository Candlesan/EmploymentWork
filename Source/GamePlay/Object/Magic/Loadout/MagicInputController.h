#pragma once
#include "System/Core/Input/GamePad.h"


enum class MagicSlotIndex { RB, RT, LB, LT, Count };
static constexpr int MAGIC_SLOT_COUNT = static_cast<int>(MagicSlotIndex::Count);

// 毎フレーム返す結果
struct MagicCommand
{
	bool press[MAGIC_SLOT_COUNT] = {}; // 押した瞬間
	bool hold[MAGIC_SLOT_COUNT] = {}; // 押している間
	bool release[MAGIC_SLOT_COUNT] = {}; // 離した瞬間
};

class MagicInputController
{
public:
	MagicCommand Update(bool isInputBlocked);

private:
	// ボタンの割り当て
	GamePadButton  slotButtons[MAGIC_SLOT_COUNT] =
	{
		GamePad::BTN_RIGHT_SHOULDER, // RB
		GamePad::BTN_RIGHT_TRIGGER,	 // RT
		GamePad::BTN_LEFT_SHOULDER,  // LB
		GamePad::BTN_LEFT_TRIGGER	 // LT
	};
};