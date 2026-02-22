#pragma once
#include <string>

// プレイヤーのアニメーション
enum class PlayerAnimationState
{
	Idle = 0,		// 待機

	// 歩き
	Walk_B,
	Walk_BL,
	Walk_BR,
	Walk_F,
	Walk_L,
	Walk_R,

	// 小走り
	Jog_B,
	Jog_BL,
	Jog_BR,
	Jog_F,
	Jog_L,
	jog_R,

	// 走り
	Run,
	Run_Stop,

	// 回避
	Roll_BL,
	Roll_BR,
	Roll_F,
	Roll_L,
	Roll_R,

	// バックステップ
	Dodge,

	// 攻撃
	Attack_01,
	Attack_02,
	Charge_Attack,
	Charge_Attack_Start,
	Charge_Attack_Loop,
	Charge_Attack_End,

	// ダッシュ攻撃
	Run_Attack,

	// ガードカウンター攻撃
	Guard_Counter,

	// ジャンプ攻撃
	Jump_Attack_Start,
	JumpAttack_Loop,
	Jump_Attack,

	// ジャンプ
	Jump_Start,
	Jump_Loop,
	Jump_End,

	// ガード（立ち）
	Guard,

	// ガード小走り
	Guard_Jog_B,
	Guard_Jog_BL,
	Guard_Jog_BR,
	Guard_Jog_F,
	Guard_Jog_L,
	Guard_Jog_R,

	// ガード歩き
	Guard_Walk_B,
	Guard_Walk_BL,
	Guard_Walk_BR,
	Guard_Walk_F,
	Guard_Walk_L,
	Guard_Walk_R,

	// ガード受け
	Guard＿Hit_01, //（小）
	Guard＿Hit_02, //（大）
	Guard＿Hit_03, //（崩れ）

	// 戦技・バフ
	Buff,

	// 武器装備
	Equip,

	// 武器解除
	UnEquip,

	// 回復
	Heal,

	// 大ダメージ
	Hurt_Heavy_B,
	Hurt_Heavy_B_Loop,
	Hurt_Heavy_F,
	Hurt_Heavy_F_Loop,
	Hurt_End_B,
	Hurt_End_F,

	// 空中ダメージ
	Hurt_In_Air_Start,
	Hurt_In_Air_Loop,
	Hurt_In_Air_End,

	// 軽ダメージ
	Hurt_Light_B,
	Hurt_Light_F,
	Hurt_Light_L,
	Hurt_Light_R,

	// アクション
	Interaction,

	// 拾う
	PickUp_01, // 何かに置いてあるものを拾う
	PickUp_02, // 地面に落ちてるものを拾う

	// 座る
	Sit_Start,
	Sit_Loop,
	Sit_End,
};

// アニメーションの設定
struct AnimationConfig
{
	std::string animationName;
	bool loop;
	bool useRootMotion;
	bool useRootMotionEx;
	float blendTime;

	AnimationConfig()
		: animationName(""), loop(false), useRootMotion(false), useRootMotionEx(false), blendTime(0.2f) {
	}

	AnimationConfig(const std::string& name, bool isLoop = false, bool rootMotion = false, bool rootMotionEx = false, float blend = 0.2f)
		: animationName(name), loop(isLoop), useRootMotion(rootMotion), useRootMotionEx(rootMotionEx), blendTime(blend) {
	}
};