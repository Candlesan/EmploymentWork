#include "FlightController.h"
#include "GamePlay/Object/Character/Character.h"
#include "System/Core/Input/Input.h"


// プレイヤーの入力用
void FlightController::ProcessInput(const FlightInput& input, float elapsedTime)
{
	// ボタンを押している間はタイマーを加算
	if (input.isAButtonDown)
	{
		aButtonHoldTime += elapsedTime;
	}
	else
	{
		aButtonHoldTime = 0.0f;
	}

	// モードごとの入力
	switch (currentMode)
	{
	case FlightMode::OFF:
		// 長押しで上昇へ
		if (aButtonHoldTime >= LONG_PRESS_TIME)
		{
			currentMode = FlightMode::UP;
		}

		break;
	case FlightMode::UP:
		// ボタンを離したら降下へ
		if (input.isAButtonUp)
		{
			currentMode = FlightMode::Down;
		}

		break;
	case FlightMode::Down:
		// 長押しで上昇へ
		if (aButtonHoldTime >= LONG_PRESS_TIME)
		{
			currentMode = FlightMode::UP;
		}
		// 短押し（離した瞬間かつ長押し未満）でホバーへ
		else if (input.isAButtonUp && aButtonHoldTime < LONG_PRESS_TIME)
		{
			currentMode = FlightMode::Hover;
		}

		break;
	case FlightMode::Hover:
		// 長押しで上昇へ
		if (aButtonHoldTime >= LONG_PRESS_TIME)
		{
			currentMode = FlightMode::UP;
		}
		// 短押しで降下へ
		else if (input.isAButtonUp && aButtonHoldTime < LONG_PRESS_TIME)
		{
			currentMode = FlightMode::Down;
		}

		break;
	}
}

// 更新処理
void FlightController::Update(Character* character, float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 飛行中ならごく微量の魔力を消費
	if (currentMode != FlightMode::OFF)
	{
		character->ConsumMagic(1.0f * elapsedTime, 0.0f);
	}

	switch (currentMode)
	{
	case FlightMode::OFF:
		// 通常時は重力を戻しておく
		character->SetGravity(-9.8f);

		break;
	case FlightMode::UP:
	{
		character->SetGravity(-9.8f); // 念のため重力を戻しておく
		DirectX::XMFLOAT3 velocity = character->GetVelocity();
		velocity.y = upSpeed; // 上昇スピードをセットする
		character->SetVelocity(velocity);

		break;
	}
	case FlightMode::Down:
		character->SetGravity(-9.8f);

		// 地面に着きそうになってるなら飛行状態をOFFにする
		if (character->GetPosition().y <= 0.05f)
		{
			currentMode = FlightMode::OFF;
		}

		break;
	case FlightMode::Hover:
	{
		// 重力と加速度のY軸を0にする
		character->SetGravity(0.0f);
		DirectX::XMFLOAT3 velocity = character->GetVelocity();
		velocity.y = 0.0f;
		character->SetVelocity(velocity);
		break;
	}
	}
}
