#pragma once

// 前方宣言
class Character;

// 飛行モード
enum class FlightMode
{
	OFF,   // 飛行していないとき
	UP,	   // 上昇
	Down,  // 降下
	Hover, // 滞空
};

// 入力状態を渡す用の構造体
struct FlightInput
{
	bool isFlightTogglePresed = false; // 飛行ON/OFFのボタン
	bool isAButtonDown = false; // Aボタンが押されたか
	bool isAButtonPresed = false; // Aボタンが押された瞬間か
	bool isAButtonUp = false; // Aボタンを離した瞬間か
	float moveX = 0.0f; // ホバー用の移動値保存用の変数
	float moveZ = 0.0f;
};

class FlightController
{
public:
	// プレイヤーの入力用
	void ProcessInput(const FlightInput& input, float elapsedTime);

	void Update(Character* character, float elapsedTime);

	// 現在の飛行モードを取得（AIの行動遷移に使うやつ）
	FlightMode GetFlightMode() const { return currentMode; }
	void SetFlightMode(FlightMode mode) { currentMode = mode; }

	// 飛行中か
	bool IsFlying() const { return currentMode != FlightMode::OFF; }

private:
	FlightMode currentMode = FlightMode::OFF; // 初期状態はOFF
	float aButtonHoldTime = 0.0f; // Aボタンをどれくらい長押ししたか
	const float LONG_PRESS_TIME = 0.25f; // 長押しとする時間

	float upSpeed = 5.0f; // 上昇速度
};