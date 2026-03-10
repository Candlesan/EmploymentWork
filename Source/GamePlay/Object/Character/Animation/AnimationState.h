#pragma once
#include <string>

// プレイヤーのアニメーション
enum class PlayerAnimationState
{
	// 待機
	Idle = 0,

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
	Jog_R,

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

	// ダッシュ攻撃
	Run_Attack,

	// ガードカウンター攻撃
	Guard_Counter,

	// ジャンプ攻撃
	Jump_Attack_Start,
	Jump_Attack_Loop,
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
	Guard_Hit_01, //（小）
	Guard_Hit_02, //（大）
	Guard_Hit_03, //（崩れ）

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

	// 死亡
	Die,
	Die_B,
	Die_F
};

// エネミーのアニメーション
enum class EnemyAnimationState
{
	// 待機
	Idle = 0,

	// 歩き
	Walk_F,
	Walk_B,
	Walk_L,
	Walk_R,

	// 小走り
	Jog_F,

	// 回避
	Dodge_Forkward,
	Dodge_Backward,
	Dodge_Left,
	Dodge_Right,

	// 通常攻撃
	Light_Attack_01,
	Light_Attack_02,
	Light_Attack_03,

	// 強攻撃
	Heavy_Attack_01,
	Heavy_Attack_02,

	// 回避(追撃)
	Dodge_FU,

	// 掴み攻撃
	Grab_Fall,

	// 叫び
	Roar,

	// 飛びつき切り
	Skill_BlockBreaker,

	// 空中2連切り(中距離からプレイヤーに向かってする攻撃)
	Skill_DoubleSwings_Root,

	// 突き攻撃(2連)
	Skill_EndlessStabs,

	// 突き攻撃(3連)
	Skill_QuickStab,

	// プレス攻撃
	Skill_HeavyStomp,

	// 突撃2連ひっかき攻撃
	Skill_Leaping,

	// 突進攻撃
	Skill_ShoulderBarge_Root,

	// アッパーカット
	Skill_UpperCut,

	// 連続切り
	Skill_WieldDagger,

	// 軽ダメージ
	Hit_Front,
	Hit_Light_Left,
	Hit_Light_Right,

	// 空中ダメージ
	Hit_Launch_Root,

	// ダウン
	Hit_Knockdown,

	// 死亡
	Death_A,
	Death_B,
};

// アニメーションの設定
struct AnimationConfig
{
	std::string animationName; // アニメーション名
	bool loop; // アニメーションをループするか
	bool useRootMotion; // ルートモーションするか
	bool useRootMotionEx;
	float blendTime; // 補間時間
	float damageRate; // 攻撃力倍率 (1.0 = 標準)
	float poiseValue;  // 体幹削り

	AnimationConfig()
		: animationName(""), loop(false), useRootMotion(false), useRootMotionEx(false),
		blendTime(0.2f), damageRate(0.0f), poiseValue(0.0f) {
	}

	// コンストラクタも更新して、デフォルト値を設定できるようにする
	AnimationConfig(const std::string& name, bool isLoop = false, bool rootMotion = false,
		bool rootMotionEx = false, float blend = 0.2f, float dmg = 0.0f, float pise = 0.0f)
		: animationName(name), loop(isLoop), useRootMotion(rootMotion),
		useRootMotionEx(rootMotionEx), blendTime(blend), damageRate(dmg), poiseValue(pise) {
	}
};