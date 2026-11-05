#pragma once
#include "GamePlay/Object/Magic/MagicInfor.h"

// 前方宣言
class MagicManager;
class LockOnSystem;

// 魔法の情報
struct MagicContext
{
	MagicSpawnInfo base;						  // owner, target, position, angleY...etc
	MagicManager* magicManager = nullptr;		  // 弾の登録先
	MagicManager* opponentMagicManager = nullptr; // 相手の魔法（迎撃・反射で使う）
	LockOnSystem* lockOn = nullptr;				  // 弾速に合わせた狙いの計算用
	DirectX::XMFLOAT3 defaultTarget = {};		  // ロックオンしていない時の狙い先
	bool canCast = false;						  // 今撃っていいか
	bool isCounterActive = false;				  // LTの技を使用中か（RTの技はこれを見て止まる）
};
