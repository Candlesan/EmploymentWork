#pragma once
#include "GamePlay/Object/Magic/MagicInfor.h"

// ‘O•ûéŒ¾
class MagicManager;
class LockOnSystem;

// –‚–@‚Ìî•ñ
struct MagicContext
{
	MagicSpawnInfo base;					// owner, target, position, angleY...etc
	MagicManager* magicManager = nullptr;	// ’e‚Ì“o˜^æ
	LockOnSystem* lockOn = nullptr;			// ’e‘¬‚É‡‚í‚¹‚½‘_‚¢‚ÌŒvZ—p
	DirectX::XMFLOAT3 defaultTarget = {};	// ƒƒbƒNƒIƒ“‚µ‚Ä‚¢‚È‚¢‚Ì‘_‚¢æ
	bool canCast = false;					// ¡Œ‚‚Á‚Ä‚¢‚¢‚©
};
