#include "GameCamera.h"
#include "System/Core/Input/Input.h"
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h" 
#include "GamePlay/Object/Character/Player/Player.h"

// 更新処理
void GameCamera::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 右スティック押し込みでロックオン切り替え
	if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_THUMB)
	{
		// ロックオン対象がいれば切り替え
		if (cameraMode == CameraMode::Normal && lockOnTarget != nullptr)
		{
			cameraMode = CameraMode::RockOn; // ロックオンへ切り替え
		}
		else
		{
			cameraMode = CameraMode::Normal; // 通常カメラに切り替え
		}
	}

	float ax = gamePad.GetAxisRX();
	float ay = gamePad.GetAxisRY();

	if (gamePad.GetButton() & GamePad::BTN_RIGHT_SHOULDER)
	{
		ax = ay = 0.0f;
	}

	// カメラの回転速度
	float speed = rollSpeed * elapsedTime;

	if (cameraMode == CameraMode::RockOn && lockOnTarget != nullptr)
	{
		// スティック入力を無視して敵の方向に角度を合わせる
		DirectX::XMFLOAT3 enemyPos = lockOnTarget->GetPosition();
		float dx = enemyPos.x - currentTarget.x;
		float dy = enemyPos.y - currentTarget.y;
		float dz = enemyPos.z - currentTarget.z;
		angle.y = std::atan2f(dx, dz);
		angle.x = -atan2f(dy, sqrtf(dx * dx + dz * dz));
	}
	else
	{
		// スティックの入力値に合わせてX軸とY軸を回転
		angle.x += ay * speed;
		angle.y += ax * speed;
	}

	// 線形補間でターゲットをジワジワ追跡させる
	// 10.0f の値を小さくするほど、揺れに対して鈍感（滑らか）になります
	float lerpSpeed = 5.0f;
	currentTarget.x += (target.x - currentTarget.x) * lerpSpeed * elapsedTime;
	currentTarget.y += (target.y - currentTarget.y) * lerpSpeed * elapsedTime;
	currentTarget.z += (target.z - currentTarget.z) * lerpSpeed * elapsedTime;

	// カメラ回転値を回転行列に変換
	DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	// 回転行列から前方向ベクトルを取り出す
	DirectX::XMVECTOR Front = Transform.r[2];
	DirectX::XMFLOAT3 front;
	DirectX::XMStoreFloat3(&front, Front);

	// 注視点から後ろベクトル方向に一定距離離れたカメラ視点を求める
	DirectX::XMFLOAT3 eye;
	eye.x = currentTarget.x - front.x * range;
	eye.y = currentTarget.y - front.y * range;
	eye.z = currentTarget.z - front.z * range;


	targetFov = defaultFov;

	std::string playerState = Player::Instance().GetCurrentState();
	bool isAvoiding = (playerState.find("Avoid") != std::string::npos ||
		playerState.find("Dodge") != std::string::npos);

	// 回避中なら、目標FOVを広くする
	if (isAvoiding)
	{
		targetFov = 65.0f;
	}

	float fovLerpSpeed = 5.0f;
	if (targetFov > currentFov)
	{
		fovLerpSpeed = 20.0f;
	}
	else
	{
		fovLerpSpeed = 5.0f;
	}

	float lerpFactor = fovLerpSpeed * elapsedTime;
	if (lerpFactor > 1.0f) lerpFactor = 1.0f;

	currentFov += (targetFov - currentFov) * lerpFactor;

	// カメラの視点と注視点を設定
	Camera& camera = CameraManager::Instance().GetMainCamera();
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(currentFov),
		camera.GetAspect(),
		camera.GetNear(),
		camera.GetFar()
	);
	camera.SetLookAt(eye, currentTarget, DirectX::XMFLOAT3(0, 1, 0));
}