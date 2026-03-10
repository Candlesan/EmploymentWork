#include "AnimationStateManager.h"


// コンストラクタ
template<>
AnimationStateManager<PlayerAnimationState>::AnimationStateManager()
{
    // 待機
    configs[PlayerAnimationState::Idle] ={ "Idle", true };

    // 歩き
    configs[PlayerAnimationState::Walk_B ]  = { "Walk_B" , true, false, true }; 
    configs[PlayerAnimationState::Walk_BL]  = { "Walk_BL", true, false, true }; 
    configs[PlayerAnimationState::Walk_BR]  = { "Walk_BR", true, false, true }; 
    configs[PlayerAnimationState::Walk_F ]  = { "Walk_F" , true, false, true };  
    configs[PlayerAnimationState::Walk_L ]  = { "Walk_L" , true, false, true }; 
    configs[PlayerAnimationState::Walk_R ]  = { "Walk_R" , true, false, true }; 

    // 小走り
    configs[PlayerAnimationState::Jog_B ] = { "Jog_B" , true };
    configs[PlayerAnimationState::Jog_BL] = { "Jog_BL", true };
    configs[PlayerAnimationState::Jog_BR] = { "Jog_BR", true };
    configs[PlayerAnimationState::Jog_F ] = { "Jog_F" , true };
    configs[PlayerAnimationState::Jog_L ] = { "Jog_L" , true };
    configs[PlayerAnimationState::Jog_R ] = { "Jog_R" , true };

    // 走り
    configs[PlayerAnimationState::Run]      = { "Run"     , true  };
    configs[PlayerAnimationState::Run_Stop] = { "Run_Stop", false, false, true };

    // 回避
    configs[PlayerAnimationState::Roll_BL] = { "Roll_BL", false, false, true };
    configs[PlayerAnimationState::Roll_BR] = { "Roll_BR", false, false, true };
    configs[PlayerAnimationState::Roll_F ] = { "Roll_F" , false, false, true };
    configs[PlayerAnimationState::Roll_L ] = { "Roll_L" , false, false, true };
    configs[PlayerAnimationState::Roll_R ] = { "Roll_R" , false, false, true };

    // バックステップ
    configs[PlayerAnimationState::Dodge] = { "Dodge", false, false, true };

    // 攻撃
    configs[PlayerAnimationState::Attack_01]           = { "Attack_01"          , false, false, true , 0.2, 1.0f, 187.0f };
    configs[PlayerAnimationState::Attack_02]           = { "Attack_02"          , false, false, true , 0.2, 1.0f, 187.0f };
    configs[PlayerAnimationState::Charge_Attack]       = { "Charge_Attack"      , false, false, true , 0.2, 1.2f, 187.0f };
    configs[PlayerAnimationState::Charge_Attack_Start] = { "Charge_Attack_Start", false, false, false, 0.2, 1.5f, 396.0f };

    // ダッシュ攻撃
    configs[PlayerAnimationState::Run_Attack] = { "Run_Attack", false, false, false, 0.2, 1.0f, 187 };

    // ガードカウンター攻撃
    configs[PlayerAnimationState::Guard_Counter] = { "Guard_Counter", false, false, true, 0.2, 2.5, 396 };

    // ジャンプ攻撃
    configs[PlayerAnimationState::Jump_Attack_Start] = { "Jump_Attack_Start", false };
    configs[PlayerAnimationState::Jump_Attack_Loop]  = { "Jump_Attack_Loop" , true  };
    configs[PlayerAnimationState::Jump_Attack]       = { "Jump_Attack"      , false, false, false, 0.2, 2.5, 264 };

    // ジャンプ
    configs[PlayerAnimationState::Jump_Start] = { "Jump_Start", false };
    configs[PlayerAnimationState::Jump_Loop]  = { "Jump_Loop" , true  };
    configs[PlayerAnimationState::Jump_End]   = { "Jump_End"  , false };

    // ガード（立ち）
    configs[PlayerAnimationState::Guard] ={ "Guard", true };

    // ガード小走り
    configs[PlayerAnimationState::Guard_Jog_B ] = { "Guard_Jog_B" , true };
    configs[PlayerAnimationState::Guard_Jog_BL] = { "Guard_Jog_BL", true };
    configs[PlayerAnimationState::Guard_Jog_BR] = { "Guard_Jog_BR", true };
    configs[PlayerAnimationState::Guard_Jog_F ] = { "Guard_Jog_F" , true };
    configs[PlayerAnimationState::Guard_Jog_L ] = { "Guard_Jog_L" , true };
    configs[PlayerAnimationState::Guard_Jog_R ] = { "Guard_Jog_R" , true };

    // ガード歩き
    configs[PlayerAnimationState::Guard_Walk_B]  = { "Guard_Walk_B" , true };
    configs[PlayerAnimationState::Guard_Walk_BL] = { "Guard_Walk_BL", true };
    configs[PlayerAnimationState::Guard_Walk_BR] = { "Guard_Walk_BR", true };
    configs[PlayerAnimationState::Guard_Walk_F]  = { "Guard_Walk_F" , true };
    configs[PlayerAnimationState::Guard_Walk_L]  = { "Guard_Walk_L" , true };
    configs[PlayerAnimationState::Guard_Walk_R]  = { "Guard_Walk_R" , true };

    // ガード受け
    configs[PlayerAnimationState::Guard_Hit_01] = { "Guard_Hit_01", false };
    configs[PlayerAnimationState::Guard_Hit_02] = { "Guard_Hit_02", false };
    configs[PlayerAnimationState::Guard_Hit_03] = { "Guard_Hit_03", false };

    // 戦技・バフ
    configs[PlayerAnimationState::Buff] = { "Buff", false };

    // 武器装備
    configs[PlayerAnimationState::Equip] = { "Equip", false };

    // 武器解除
    configs[PlayerAnimationState::UnEquip] = { "UnEquip", false };

    // 回復
    configs[PlayerAnimationState::Heal] = { "Heal", false };

    // 大ダメージ
    configs[PlayerAnimationState::Hurt_Heavy_B]      = { "Hurt_Heavy_B"     , false };
    configs[PlayerAnimationState::Hurt_Heavy_B_Loop] = { "Hurt_Heavy_B_Loop", true  };
    configs[PlayerAnimationState::Hurt_Heavy_F]      = { "Hurt_Heavy_F"     , false };
    configs[PlayerAnimationState::Hurt_Heavy_F_Loop] = { "Hurt_Heavy_F_Loop", true  };
    configs[PlayerAnimationState::Hurt_End_B]        = { "Hurt_End_B"       , false };
    configs[PlayerAnimationState::Hurt_End_F]        = { "Hurt_End_F"       , false };

    // 空中ダメージ
    configs[PlayerAnimationState::Hurt_In_Air_Start] = { "Hurt_In_Air_Start", false };
    configs[PlayerAnimationState::Hurt_In_Air_Loop]  = { "Hurt_In_Air_Loop" , false };
    configs[PlayerAnimationState::Hurt_In_Air_End]   = { "Hurt_In_Air_End"  , false };

    // 大ダメージ
    configs[PlayerAnimationState::Hurt_Light_B] = { "Hurt_Light_B", false };
    configs[PlayerAnimationState::Hurt_Light_F] = { "Hurt_Light_F", true  };
    configs[PlayerAnimationState::Hurt_Light_L] = { "Hurt_Light_L", false };
    configs[PlayerAnimationState::Hurt_Light_R] = { "Hurt_Light_R", true  };

    // アクション
    configs[PlayerAnimationState::Interaction] = { "Interaction", false };

    // 拾う
    configs[PlayerAnimationState::PickUp_01] = { "PickUp_01", false };
    configs[PlayerAnimationState::PickUp_02] = { "PickUp_02", false };

    // 座る
    configs[PlayerAnimationState::Sit_Start] = { "Sit_Start", false };
    configs[PlayerAnimationState::Sit_Loop]  = { "Sit_Loop" , true  };
    configs[PlayerAnimationState::Sit_End]   = { "Sit_End"  , false };

    // 死亡
    configs[PlayerAnimationState::Die]     = { "Die"    , false };
    configs[PlayerAnimationState::Die_B]   = { "Die_B"  , false };
    configs[PlayerAnimationState::Die_F]   = { "Die_F"  , false };
}

// コンストラクタ
template<>
AnimationStateManager<EnemyAnimationState>::AnimationStateManager()
{
    // 待機
    configs[EnemyAnimationState::Idle] ={ "Idle", true };

    // 歩き
    configs[EnemyAnimationState::Walk_B ]  = { "Walk_B" , true, false, true }; 
    configs[EnemyAnimationState::Walk_F ]  = { "Walk_F" , true, false, true };  
    configs[EnemyAnimationState::Walk_L ]  = { "Walk_L" , true, false, true }; 
    configs[EnemyAnimationState::Walk_R ]  = { "Walk_R" , true, false, true }; 

    // 小走り
    configs[EnemyAnimationState::Jog_F ] = { "Jog_F" , true };

    // 回避
    configs[EnemyAnimationState::Dodge_Forkward] = { "Dodge_Forkward" , false, false, true };
    configs[EnemyAnimationState::Dodge_Backward] = { "Dodge_Backward" , false, false, true };
    configs[EnemyAnimationState::Dodge_Left] =     { "Dodge_Left"     , false, false, true };
    configs[EnemyAnimationState::Dodge_Right] =    { "Dodge_Right"    , false, false, true };

    // 通常攻撃
    configs[EnemyAnimationState::Light_Attack_01] = { "Light_Attack_01" , false, false, true };
    configs[EnemyAnimationState::Light_Attack_02] = { "Light_Attack_02" , false, false, true };
    configs[EnemyAnimationState::Light_Attack_03] = { "Light_Attack_03" , false, false, true };

    // 強攻撃
    configs[EnemyAnimationState::Heavy_Attack_01] = { "Heavy_Attack_01" , false, false, true };
    configs[EnemyAnimationState::Heavy_Attack_02] = { "Heavy_Attack_02" , false, false, true };

    // 回避(追撃)
    configs[EnemyAnimationState::Dodge_FU] = { "Dodge_FU" , false, false, true };

    // 掴み攻撃
    configs[EnemyAnimationState::Grab_Fall] = { "Grab_Fall" , false, false, true };

    // 叫び
    configs[EnemyAnimationState::Roar] = { "Roar" , false, false, true };

    // 飛びつき切り
    configs[EnemyAnimationState::Skill_BlockBreaker] = { "Skill_BlockBreaker" , false, false, true };

    // 空中2連切り(中距離からプレイヤーに向かってする攻撃)
    configs[EnemyAnimationState::Skill_DoubleSwings_Root] = { "Skill_DoubleSwings_Root" , false, false, true };

    // 突き攻撃(2連)
    configs[EnemyAnimationState::Skill_EndlessStabs] = { "Skill_EndlessStabs" , false, false, true };

    // 突き攻撃(3連)
    configs[EnemyAnimationState::Skill_QuickStab] = { "Skill_QuickStab" , false, false, true };

    // プレス攻撃
    configs[EnemyAnimationState::Skill_HeavyStomp] = { "Skill_HeavyStomp" , false, false, true };

    // 突撃2連ひっかき攻撃
    configs[EnemyAnimationState::Skill_Leaping] = { "Skill_Leaping" , false, false, true };

    // 突進攻撃
    configs[EnemyAnimationState::Skill_ShoulderBarge_Root] = { "Skill_ShoulderBarge_Root" , false, false, true };

    // アッパーカット
    configs[EnemyAnimationState::Skill_UpperCut] = { "Skill_UpperCut" , false, false, true };

    // 連続切り
    configs[EnemyAnimationState::Skill_WieldDagger] = { "Skill_WieldDagger" , false, false, true };

    // 軽ダメージ
    configs[EnemyAnimationState::Hit_Front] =       { "Hit_Front"       , false, false, true };
    configs[EnemyAnimationState::Hit_Light_Left] =  { "Hit_Light_Left"  , false, false, true };
    configs[EnemyAnimationState::Hit_Light_Right] = { "Hit_Light_Right" , false, false, true };

    // 空中ダメージ
    configs[EnemyAnimationState::Hit_Launch_Root] = { "Hit_Launch_Root" , false, false, true };

    // ダウン
    configs[EnemyAnimationState::Hit_Knockdown] = { "Hit_Knockdown" , false, false, true };

    // 死亡
    configs[EnemyAnimationState::Death_A] = { "Death_A" , false, false, true };
    configs[EnemyAnimationState::Death_B] = { "Death_B" , false, false, true };
}