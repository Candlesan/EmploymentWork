#pragma once
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/ActionBase.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"

#include "System/UI/EffectEditor/EffectManager/CustomEffect.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"


// 待機行動
class Idle : public ActionBase
{
public:
	Idle(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 前進
class MoveForward : public ActionBase
{
public:
	MoveForward(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 後退
class MoveBack : public ActionBase
{
public:
	MoveBack(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);
};

// 右
class MoveRight : public ActionBase
{
public:
	MoveRight(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);

private:
	float strafeDir = 1.0f;
	float timer = 0.0f;
};

// 左
class MoveLeft : public ActionBase
{
public:
	MoveLeft(Enemy* enemy, std::string anim) : ActionBase(enemy, anim) {}
	ActionBase::State Run(float elapsedTime);

private:
	float strafeDir = -1.0f;
	float timer = 0.0f;
};

// 回避
class Avoid : public ActionBase
{
public:
	Avoid(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
	void Abort() override
	{
		owner->SetIsAvoid(false);
		isStarted = false;
	}

private:
	bool isStarted = false;
	DirectX::XMFLOAT3 avoidDir = { 0, 0, 1 }; // 回避する向き
};

// ========= 魔法関係 =========
// 通常攻撃魔法
class NormalMagicAttack : public ActionBase
{
public:
	NormalMagicAttack(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
	void Abort() override 
	{
		phase = Phase::Start; 
		hasFired = false;
		owner->StopOverlayAnimation();
	}

private:
	enum class Phase
	{
		Start,
		Attacking
	};

	Phase phase = Phase::Start;
	NormalMagic normalMagic;
	NormalMagic::NormalMagicConfig magicConfig;
	bool hasFired = false; // 既に魔法を発射したかのフラグ
};

// 連射魔法
class RapidFireMagic : public ActionBase
{
public:
	RapidFireMagic(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);

	void Abort() override
	{
		phase = Phase::Start;
		owner->SetIsRapidFire(false);
		owner->StopOverlayAnimation();
	}
private:
	enum class Phase
	{
		Start,
		Attacking
	};

	Phase phase = Phase::Start;
	RapidMagic rapidMagic;
	RapidMagic::RapidMagicConfig magicConfig;
	float fireTimer = 0.0f;     // 撃ち続ける残り時間
};

// 防御魔法
class GuardMagic : public ActionBase
{
public:
	GuardMagic(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
	void Abort() override
	{
		owner->SetIsGaurd(false);
		owner->GetDefenseMagic().SetIsActive(false);
		defenseEffect->Stop();
		defenseEffect = nullptr;
	}

private:
	CustomEffect* defenseEffect = nullptr;
};

// 防御解除（回復するまで張らない）
class DropGuardMagic : public ActionBase
{
public:
	DropGuardMagic(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);
};

// 迎撃
class InterceptMagic : public ActionBase
{
public:
	InterceptMagic(Enemy* enemy, std::string stateName) : ActionBase(enemy, stateName) {}
	ActionBase::State Run(float elapsedTime);

	void Abort() override {
		owner->SetIsIntercept(false);
		owner->SetIsRapidFire(false);
		owner->StopOverlayAnimation();
		phase = Phase::Start;
		hasFired = false;
		fireTimer = 0.0f;
	}

private:
	enum class Phase
	{
		Start,
		Attacking
	};

	Phase phase = Phase::Start;
	NormalMagic normalMagic;
	NormalMagic::NormalMagicConfig normalConfig;
	RapidMagic rapidMagic;
	RapidMagic::RapidMagicConfig rapidFireConfig;
	bool hasFired = false; // 既に魔法を発射したかのフラグ
	float fireTimer = 0.0f;     // 撃ち続ける残り時間
};