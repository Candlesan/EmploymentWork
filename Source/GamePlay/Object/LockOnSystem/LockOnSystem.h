#pragma once
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"

#include "GamePlay/Object/Character/Player/PlayerUI.h"
#include "GamePlay/Object/Camera/GameCamera/GameCamera.h"

#include <DirectXMath.h>

// 前方宣言
class Camera;
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

	// 予測位置を返す
	DirectX::XMFLOAT3 LockOnSystem::GetPredictedPosition(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const;

	// 偏差込みの発射方向(主に敵用)
	DirectX::XMFLOAT3 GetAimDirection(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const;

	bool IsLocked() const { return state == State::Locked; } // ロックオン中か
	State GetState() const { return state; }
	Character* GetTarget() const { return IsLocked() ? target : nullptr; } // ターゲットがいれば返す。いなければnullptrを返す
	float GetLockProgress() const; // UIFillに使うやつ

	// ハードロック
	void SetHardLock(bool flag) { hardLock = flag; }
	bool IsHardLock() const { return hardLock; }

	// 減速型エイムアシスト
	float AimAssistScale();

	// 追尾型エイムアシスト
	DirectX::XMFLOAT2 GetTarckingAssist(const DirectX::XMFLOAT3& origin, bool hasInput);

	
	bool GetAuotAim() const { return autoAim; }
	void SetAuotAim(bool flag) { autoAim = flag; }
private:
	// ロックオンが出来るか
	bool IsTargetable(const DirectX::XMFLOAT3& aimDirection) const;

	// 敵が画面内にいるか
	bool IsEnemyInScreen(const DirectX::XMFLOAT3& worldPos, const Camera& camera) const;
private:
	Character* owner = nullptr;
	Character* target = nullptr;
	PlayerUI ui;
	
	// パラメーター
	State state = State::None;
	DirectX::XMFLOAT3 aimDirection = { 0, 0, 1 }; // 初期値は正面
	float lockTimer = 0.0f; // ロックオン判定になるまでの時間
	float assistAngle = 10.0f; // アシスト判定になる角度
	bool hardLock = false; // 敵が画面中央に固定されるフラグ

	// GUI用
	float range = 100.0f; // 自動ロックオン射程距離
	float lockAngle = 45.0f; // ロックオン判定の角度
	float lockTime = 0.3; // ロックオン判定になるまでの時間(秒)
	float leadError = 0.0f; // 偏差の誤差
	bool autoAim = true; // オートエイムを使うか

	DirectX::XMFLOAT2 prevTargetAngle = { 0, 0 }; // 前のフレームの角度
	bool hasPrevAngle = false; // 前フレームの角度があるか
	float trackingStrength = 1.0f; // 追従の強さ
};