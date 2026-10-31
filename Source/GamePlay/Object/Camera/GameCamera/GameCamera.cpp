#include "GameCamera.h"
#include "System/Core/Input/Input.h"
#include "System/Core/Mathf.h"

#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h" 
#include "GamePlay/Object/Character/Player/Player.h"

#include <imgui.h>
#include <algorithm>

// 更新処理
void GameCamera::Update(float elapsedTime, const std::vector<Collision::Triangle>& stageTriangles, const std::vector<Collision::Area>& stageAreas)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Player& player = Player::Instance();
	float ax = gamePad.GetAxisRX();
	float ay = -gamePad.GetAxisRY();

	// Y軸反転がOnの時はリバースさせる
	if (isInvertY)
	{
		ay = -ay;
	}

	// カメラの回転速度(エイムアシスト補正を入れる)
	float assist = player.GetLockOnSystem().AimAssistScale();
	float slow = 1.0f - assistStrength * assist;
	float targetVelX = ay * rollSpeed * slow;
	float targetVelY = ax * rollSpeed * slow;

	bool hasInput = fabsf(ax) > 0.1f || fabsf(ay) > 0.1f; // 少しでも動かしていたらtrue
	DirectX::XMFLOAT3 v = player.GetVelocity();
	bool isMoving = (v.x * v.x + v.y * v.y + v.z * v.z) > 1.0f; // 秒速1m以上で移動中
	float inputRate = hasInput ? 1.0f : (isMoving ? 0.5f : 0.0f);
	DirectX::XMFLOAT2 track = player.GetLockOnSystem().GetTarckingAssist(player.GetPosition(), inputRate);
	angle.x += track.x;
	angle.y += track.y;
	
	// expf(-速さ * elapsedTime)必ず 0〜1 の範囲に収まる
	float rotT = 1.0f - expf(-rotSpeed * elapsedTime);
	currentRotVel.x = Mathf::Lerp(currentRotVel.x, targetVelX, rotT);
	currentRotVel.y = Mathf::Lerp(currentRotVel.y, targetVelY, rotT);

	if (player.GetLockOnSystem().IsHardLock()
		&& player.GetLockOnSystem().GetTarget() != nullptr)
	{
		// スティック入力を無視して敵の方向に角度を合わせる
		DirectX::XMFLOAT3 enemyPos = player.GetLockOnSystem().GetTarget()->GetPosition();
		float dx = enemyPos.x - currentTarget.x;
		float dy = enemyPos.y - currentTarget.y;
		float dz = enemyPos.z - currentTarget.z;

		// 目標の角度
		float targetYaw = std::atan2f(dx, dz);
		float targetPitch = -atan2f(dy, sqrtf(dx * dx + dz * dz));

		float lockOnT = 1.0f - expf(-lockOnLerpSpeed * elapsedTime);

		// 縦はそのまま補間
		angle.x = Mathf::Lerp(angle.x, targetPitch, lockOnT);

		// 横は -π〜π の境目で逆回りしないように差分を丸めてから補間
		float diff = targetYaw - angle.y;
		while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
		while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;
		angle.y = Mathf::Lerp(angle.y, angle.y + diff, lockOnT);
	}
	else
	{
		// スティックの入力値に合わせてX軸とY軸を回転
		angle.x += currentRotVel.x * elapsedTime;
		angle.y += currentRotVel.y * elapsedTime;

		// カメラの上下の制限をかける
		const float MAX_ANGLE = DirectX::XMConvertToRadians(80.0f);

		if (angle.x > MAX_ANGLE)
		{
			angle.x = MAX_ANGLE;
		}
		else if (angle.x < -MAX_ANGLE)
		{
			angle.x = -MAX_ANGLE;
		}
	}

	float speed = Mathf::Length(player.GetVelocity());
	float followSpeed = (std::max)(lerpSpeed, speed / maxLag);

	// 線形補間でターゲットをジワジワ追跡させる
	// 10.0f の値を小さくするほど、揺れに対して鈍感（滑らか）になります
	currentTarget.x += (target.x - currentTarget.x) * followSpeed * elapsedTime;
	currentTarget.y += (target.y - currentTarget.y) * followSpeed * elapsedTime;
	currentTarget.z += (target.z - currentTarget.z) * followSpeed * elapsedTime;

	// カメラ回転値を回転行列に変換
	DirectX::XMMATRIX Transform = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	// 回転行列から前方向ベクトルを取り出す
	DirectX::XMVECTOR Front = Transform.r[2];
	DirectX::XMFLOAT3 front;
	DirectX::XMStoreFloat3(&front, Front);

	// カメラの右方向
	DirectX::XMFLOAT3 right;
	DirectX::XMStoreFloat3(&right, Transform.r[0]);

	// カメラ基準でずらした注視点(出発点＋方向×動かす量)
	DirectX::XMFLOAT3 pivot;
	pivot.x = currentTarget.x + right.x * offset.x + front.x * offset.z;
	pivot.y = currentTarget.y + offset.y + front.y * offset.z;
	pivot.z = currentTarget.z + right.z * offset.x + front.z * offset.z;

	DirectX::XMFLOAT3 eye;

	// スタート地点を設定
	DirectX::XMFLOAT3 origin = pivot;

	// 飛ばす方向を設定(カメラと反対向き)
	DirectX::XMFLOAT3 direction = { -front.x, -front.y, -front.z };

	// 飛ばす距離
	float distance = range;

	DirectX::XMFLOAT3 hitPosition, hitNormal;

	// ステージとの当たり判定
	float desiredDistance = range; // 今フレームの目標距離を決める
	if (Collision::SphereCast(origin, direction, radius, distance, stageTriangles, stageAreas, hitPosition, hitNormal, true)) 
	{
		// カメラとターゲットの距離を測る
		float dx = hitPosition.x - pivot.x;
		float dy = hitPosition.y - pivot.y;
		float dz = hitPosition.z - pivot.z;

		// 距離が0以下になったら0.1にする
		 desiredDistance = (std::max)(sqrtf(dx * dx + dy * dy + dz * dz), 0.1f);
	}

	if (desiredDistance < currentDistance)
	{
		// 壁に近づくときは即座に合わせる（めり込み防止）
		currentDistance = desiredDistance;
	}
	else
	{
		// 離れる時はゆっくり戻す
		float returnSpeed = 3.0f;
		currentDistance += (desiredDistance - currentDistance) * (1.0f - expf(-returnSpeed * elapsedTime));
	}

	// 注視点から後ろベクトル方向に一定距離離れたカメラ視点を求める
	eye.x = pivot.x - front.x * currentDistance;
	eye.y = pivot.y - front.y * currentDistance;
	eye.z = pivot.z - front.z * currentDistance;

	targetFov = defaultFov;

	std::string playerState = Player::Instance().GetCurrentState();
	bool isAvoiding = (playerState.find("Avoid") != std::string::npos ||
		playerState.find("Dodge") != std::string::npos);

	// 回避中なら、目標FOVを広くする
	if (isAvoiding)
	{
		targetFov = maxFov;
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
	camera.SetLookAt(eye, pivot, DirectX::XMFLOAT3(0, 1, 0));
}

void GameCamera::DrawGUI()
{
	if (ImGui::CollapsingHeader(u8"調整"))
	{
		ImGui::DragFloat3(u8"オフセット", &offset.x, 0.01f, -3.0f, 3.0f);
		ImGui::SliderAngle(u8"回転速度", &rollSpeed, 0.0f, 360.0f);
		ImGui::DragFloat(u8"回転の追従", &rotSpeed, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat(u8"距離", &range, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat(u8"ロックオン追従", &lockOnLerpSpeed, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat(u8"注視点追従", &lerpSpeed, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat(u8"注視点追従補正", &maxLag, 0.1f, 0.0f, 50.0f);
		ImGui::DragFloat(u8"当たり判定半径", &radius, 0.01f, 0.0f, 2.0f);
		ImGui::DragFloat(u8"通常FOV", &defaultFov, 0.1f, 10.0f, 120.0f);
		ImGui::DragFloat(u8"回避FOV", &maxFov, 0.1f, 10.0f, 120.0f);
	}

	if (ImGui::CollapsingHeader(u8"エイムアシスト"))
	{
		ImGui::Text(u8"アシスト: %.2f", Player::Instance().GetLockOnSystem().AimAssistScale());
		ImGui::DragFloat(u8"エイムアシストの強さ", &assistStrength, 0.1f, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader(u8"状態"))
	{
		ImGui::Text(u8"角度  : %.1f, %.1f",
			DirectX::XMConvertToDegrees(angle.x), DirectX::XMConvertToDegrees(angle.y));
		ImGui::Text(u8"回転速度: %.2f, %.2f", currentRotVel.x, currentRotVel.y);
		ImGui::Text(u8"FOV   : %.1f -> %.1f", currentFov, targetFov);
	}
}