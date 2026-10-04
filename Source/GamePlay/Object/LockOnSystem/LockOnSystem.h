#pragma once
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"

#include "GamePlay/Object/Character/Player/PlayerUI.h"

#include <DirectXMath.h>

// 前方宣言
class Character;


class LockOnSystem
{
public:
	// ロックオンの状態
	enum class State {None, locking, Locked};

	// 初期化
	void Initialize(Character* owner, Character* target);

	// aimDirection：プレイヤーなら正面、敵ならAIの向き
	void Update(float elapsedTime, const DirectX::XMFLOAT3& direction);

	// GUI描画
	void DrawGUI();

	DirectX::XMFLOAT3 LockOnSystem::GetPredictedPosition(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const;

	// 偏差込みの発射方向（ロックしていなければaimDirectionをそのまま返す）
	DirectX::XMFLOAT3 GetAimDirection(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const;

	bool IsLocked() const { return state == State::Locked; } // ロックオン中か
	State GetState() const { return state; }
	Character* GetTarget() const { return IsLocked() ? target : nullptr; } // ターゲットがいれば返す。いなければnullptrを返す
	float GetLockProgress() const; // UIFillに使うやつ

	// ハードロック
	void SetHardLock(bool flag) { hardLock = flag; }
	bool IsHardLock() const { return hardLock; }
private:
	// ロックオンが出来るか
	bool IsTargetable(const DirectX::XMFLOAT3& aimDirection) const;

private:
	Character* owner = nullptr;
	Character* target = nullptr;
	PlayerUI ui;
	
	// パラメーター
	State state = State::None;
	DirectX::XMFLOAT3 aimDirection = { 0, 0, 1 }; // 初期値は正面
	float lockTimer = 0.0f; // ロックオン判定になるまでの時間
	bool hardLock = false; // 敵が画面中央に固定されるフラグ

	// GUI用
	float range = 100.0f; // 自動ロックオン射程距離
	float lockAngle = 20.0f; // ロックオン判定の角度
	float lockTime = 0.3; // ロックオン判定になるまでの時間(秒)
	float leadError = 0.0f; // 偏差の誤差
};