#pragma once
#include <string>
#include <vector>
#include <DirectXMath.h>

enum class TransitionConditionType
{
	AnimationFinished,	// アニメーションが終了したら
	AnimationTimeOver,	// 指定の秒数を超えたら
	AnimationTimeIn,	// 指定の秒数以内だったら

	ButtonPressed,		// ボタンが押されたら
	ButtonReleased,		// ボタンを離したら
	ButtonHeld,			// ボタンを長押ししていたら

	MoveLengthOver,		// スティックの入力値が一定以上だったら
	MoveLengthUnder,	// スティックの入力値が一定以下だったら
	InputDir,			// 入力方向

	EnemyFromDistance,	// 敵との距離

	IsGround,			//	地面にいるかどうか

	IsLockOn,			// ロックオン中
	IsGuarding,			// ガード中

	PositionX,			// 位置:X座標
	PositionY,			// 位置:Y座標
	PositionZ,			// 位置:Z座標

	IsCancelable,		// モーションキャンセル
	IsFalling,			// 落下中か

	Always,				// 無条件
};

enum class TransitionActionType
{
	None,
	MoveSpeed, // 移動速度
	TurnSpeed, // 旋回速度
	SetIsAvoid, // 回避フラグ
};

enum class RelativeInputDir
{
	None,			// 入力なし
	Forward,		// 前
	ForwardRight,	// 右斜め前
	ForwardLeft,	// 左斜め前
	Right,			// 右
	Backward,		// 後ろ
	BackwardRight,	// 後ろ斜め後ろ
	BackwardLeft,	// 後ろ斜め後ろ
	Left			// 左
};

struct TransitionAction
{
	TransitionActionType type = TransitionActionType::None;
	float value = 0.0f; // ConsumeStaminaの時の消費量
};

struct TransitionCondition
{
	TransitionConditionType type = TransitionConditionType::AnimationFinished;
	float threshold = 0.0f;    // 時間・入力量などの閾値
	int   buttonMask = 0;      // GamePad::BTN_XXX
	bool  negate = false;      // 条件を反転（〜でない）
	RelativeInputDir targetDir = RelativeInputDir::None;
};

struct AnimationTransition
{
	std::string fromState;  // PlayerAnimationState の int キャスト
	std::string toState;
	std::vector<TransitionCondition> conditions;  // AND条件
	int priority = 0;  // 複数遷移が成立したとき優先度が高いほうが勝つ
	std::vector<TransitionAction> actions;
};

enum class InputSlot { A, B };

struct TransitionContext
{
	DirectX::XMFLOAT3 position;
	float animSeconds;
	float animLength;
	float moveLength;
	float enemyFromdistance;
	float HoldTime;
	int   buttonDown;
	int   buttonHeld;
	int   buttonUp;
	bool  IsGround;
	bool  isLockOn;
	bool  isGuarding;
	bool IsCancelable;
	bool IsFalling;
	RelativeInputDir inputDir;
};