#include "MagicInputController.h"
#include "System/Core/Input/Input.h"

// 更新処理(GamePadを読んでMagicCommandを作る（1フレームに1回だけ呼ぶ）)
MagicCommand MagicInputController::Update(bool isInputBlocked)
{
	MagicCommand cmd;
	if (isInputBlocked) return cmd; // 文字入力中は処理を受け付けない

	GamePad& gamePad = Input::Instance().GetGamePad();
	for (int i = 0; i < MAGIC_SLOT_COUNT; ++i)
	{
		cmd.press[i] = ((gamePad.GetButtonDown() & slotButtons[i]) != 0);
		cmd.hold[i] = ((gamePad.GetButton() & slotButtons[i]) != 0);
		cmd.release[i] = ((gamePad.GetButtonUp() & slotButtons[i]) != 0);
	}

	return cmd;
}
