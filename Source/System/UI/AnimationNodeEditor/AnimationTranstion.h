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

	BHold,				// Bボタン長押し（走り）
	BTap,				// Bボタン短押し（回避）
	RBTap,				// Bボタン短押し（バックステップ）

	RTHold,				// RT長押し（溜め攻撃）
	RTTap,				// RT短押し（強攻撃）

	JumpPressed,		// ジャンプ入力
	StaminaEmpty,		// スタミナ切れ
	HasStamina,			// スタミナあり

	IsLockOn,			// ロックオン中
	CanRun,				// 走れる状態
	HavePotion,			// ポーションがある
	HealCooldownReady,	// 回復のクールダウン中か
	IsGuarding,			// ガード中

	PositionX,			// 位置:X座標
	PositionY,			// 位置:Y座標
	PositionZ,			// 位置:Z座標
	Always,				// 無条件
};

enum class TransitionActionType
{
	None,
	MoveSpeed, // 移動速度
	TurnSpeed, // 旋回速度
	ConsumeStamina, // スタミナ消費
	SetIsAvoid, // 回避フラグ
	SetAnimationSpeed, // アニメーション再生速度
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
};

struct AnimationTransition
{
	std::string fromState;  // PlayerAnimationState の int キャスト
	std::string toState;
	std::vector<TransitionCondition> conditions;  // AND条件
	int priority = 0;  // 複数遷移が成立したとき優先度が高いほうが勝つ
	std::vector<TransitionAction> actions;
};

struct TransitionContext
{
	DirectX::XMFLOAT3 position;
	float animSeconds;
	float animLength;
	float moveLength;
	int   buttonDown;
	int   buttonHeld;
	int   buttonUp;
	bool  bHold;
	bool  bTap;
	bool  rbTap;
	bool  rtHold;
	bool  rtTap;
	bool  jumpPressed;
	bool  haveStamina;
	bool  isStaminaEmpty;
	bool  canRunTrigger;
	bool  isLockOn;
	bool  havePotion;
	bool  healCooldownReady;
	bool  isGuarding;
};