#include "GameCamera.h"
#include "System/Core/Input/Input.h"
#include "GamePlay/Object/Camera/Camera.h"

// 更新処理
void GameCamera::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisRX();
	float ay = gamePad.GetAxisRY();
	// カメラの回転速度
	float speed = rollSpeed * elapsedTime;

	// スティックの入力値に合わせてX軸とY軸を回転
	angle.x += ay * speed;
	angle.y += ax * speed;

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

	// カメラの視点と注視点を設定
	Camera& camera = CameraManager::Instance().GetMainCamera();
	camera.SetLookAt(eye, currentTarget, DirectX::XMFLOAT3(0, 1, 0));
}