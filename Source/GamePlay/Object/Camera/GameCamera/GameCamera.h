#pragma once
#include <DirectXMath.h>

// 前方宣言
class Enemy;

// ゲーム用のカメラ
class GameCamera
{
public:
	// 更新処理
	void Update(float elapsedTime);

	// ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& target) { this->target = target; }

	// カメラとの距離をセット
	void SetRange(float r) { range = r; }

	// カメラの角度をセット
	void SetAngle(DirectX::XMFLOAT3 A) { angle = A; }

	// ロックオン対象を外から渡す
	void SetLockOnTarget(Enemy* e) { lockOnTarget = e; }
	Enemy* GetLockOnTarget() const { return lockOnTarget; }
	bool IsLockOn() const { return cameraMode == CameraMode::RockOn; }

private:
	enum class CameraMode 
	{
		Normal, // 通常攻撃
		RockOn  // ロックオン
	};
	CameraMode cameraMode = CameraMode::Normal;


	Enemy* lockOnTarget = nullptr; // ← 追加
	DirectX::XMFLOAT3 target = { 0, 0, 0 }; // 注視点
	DirectX::XMFLOAT3 currentTarget = { 0, 0, 0 };
	DirectX::XMFLOAT3 angle = { 0, 0, 0 }; // 回転角度
	float rollSpeed = DirectX::XMConvertToRadians(90); // 回転速度
	float range = 10.0f; // 距離
};

