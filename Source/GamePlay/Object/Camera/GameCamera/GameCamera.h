#pragma once
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"
#include <DirectXMath.h>

// 前方宣言
class Enemy;

// ゲーム用のカメラ
class GameCamera
{
public:
	// 更新処理
	void Update(float elapsedTime, const std::vector<Collision::Triangle>& stageTriangles, const std::vector<Collision::Area>& stageAreas);

	// ターゲット位置設定
	void SetTarget(const DirectX::XMFLOAT3& target) { this->target = target; }

	// カメラとの距離をセット
	void SetRange(float r) { range = r; }

	// カメラの角度をセット
	void SetAngle(DirectX::XMFLOAT3 A) { angle = A; }

	// ロックオン対象を外から渡す
	void SetInvertY(bool y) { isInvertY = y; }

	void DrawGUI();

private:
	DirectX::XMFLOAT3 target = { 0, 0, 0 }; // 注視点
	DirectX::XMFLOAT3 currentTarget = { 0, 0, 0 };
	DirectX::XMFLOAT3 angle = { 0, 0, 0 }; // 回転角度
	DirectX::XMFLOAT2 currentRotVel = { 0.0f, 0.0f }; // x:縦回転, y:横回転
	float rollSpeed = DirectX::XMConvertToRadians(120); // 回転速度
	float rotSpeed = 15.0f;
	float lockOnLerpSpeed = 10.0f;
	float lerpSpeed = 5.0f;
	float range = 10.0f; // 距離
	float currentDistance = range;

	float currentFov = 45.0f; // 今の実際のFOV
	float defaultFov = 45.0f; // 通常時のFOV
	float maxFov = 65.0f; // 最大FOV
	float targetFov = 0.0f;
	
	float radius = 0.2f; // カメラの当たり判定
	float assistStrength = 0.7; // アシストの強さ(0～1)

	bool isInvertY = false; // true：リバース / false：ノーマル
};