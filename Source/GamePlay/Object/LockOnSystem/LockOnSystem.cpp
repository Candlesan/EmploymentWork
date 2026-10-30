#include "LockOnSystem.h"

#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Character.h"

#include <algorithm>
#include <imgui.h>

// 初期化
void LockOnSystem::Initialize(Character* owner, Character* target)
{
	this->owner = owner;
	this->target = target;
}

// 更新処理
void LockOnSystem::Update(float elapsedTime, const DirectX::XMFLOAT3& direction)
{
	if (owner == nullptr || target == nullptr) return;

	aimDirection = direction;
	if (!autoAim) { state = State::None; return; }


	// ロックが可能ならタイマーを増やす
	if (IsTargetable(aimDirection))
	{
		lockTimer += elapsedTime;

		// 指定した時間を超えていたらロック完了
		if (lockTimer >= lockTime)
		{
			lockTimer = lockTime;
			state = State::Locked;
		}
	}
	// 条件が外れたらタイマーを外す
	else
	{
		// タイマーを減らす
		lockTimer -= elapsedTime;

		if (lockTimer < 0.0f)
		{
			lockTimer = 0.0f;
			state = State::None;
		}
	}
}

void LockOnSystem::DrawGUI()
{
	if (ImGui::CollapsingHeader(u8"ロックオンシステム", ImGuiTreeNodeFlags_DefaultOpen))
	{
		switch (state)
		{
		case LockOnSystem::State::None:
			ImGui::Text(u8"状態:None");
			break;
		case LockOnSystem::State::locking:
			ImGui::Text(u8"状態:locking");
			break;
		case LockOnSystem::State::Locked:
			ImGui::Text(u8"状態:Locked");
			break;
		}

		ImGui::Checkbox(u8"オートエイム", &autoAim);
		ImGui::DragFloat(u8"ロックオンの感知距離", &range, 0.1f);
		ImGui::DragFloat(u8"ロックオンの感知角度", &lockAngle, 0.1f);
		ImGui::DragFloat(u8"ターゲット補足時間", &lockTime, 0.1f);
		ImGui::Separator();

		ImGui::DragFloat(u8"エイムアシストの感知角度", &assistAngle, 0.1f);
		ImGui::DragFloat(u8"追尾の強さ", &trackingStrength, 0.1f);

		ImGui::Separator();
		ui.DrawGUI();
	}
}

// 予測位置を返す
DirectX::XMFLOAT3 LockOnSystem::GetPredictedPosition(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const
{
	if (!target) return { 0, 0, 0 };

	DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&target->GetPosition());
	E = DirectX::XMVectorAdd(E, DirectX::XMVectorSet(0, 0.7f, 0, 0)); // 狙う位置を敵の腰当たりにする
	DirectX::XMVECTOR V = DirectX::XMLoadFloat3(&target->GetVelocity());
	DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&muzzlePos);
	DirectX::XMVECTOR D = DirectX::XMVectorSubtract(E, P);

	// 判別式の準備をする
	float a = DirectX::XMVectorGetX(DirectX::XMVector3Dot(V, V)) - (projectileSpeed * projectileSpeed);
	float b = 2.0f * DirectX::XMVectorGetX(DirectX::XMVector3Dot(D, V));
	float c = DirectX::XMVectorGetX(DirectX::XMVector3Dot(D, D));

	float t = -1;
	float t1 = -1;
	float t2 = -1;
	float disc = 0.0f;
	// aがほぼ0なら1次方程式
	if (std::fabsf(a) < 0.001f)
	{
		if (std::fabsf(b) >= 0.001f) t = -c / b;
	}
	// 判別式
	else
	{
		disc = b * b - 4 * a * c;
		if (disc >= 0.0f)
		{
			t1 = (((b * -1.0f) - std::sqrtf(disc)) / (2.0f * a));
			t2 = (((b * -1.0f) + std::sqrtf(disc)) / (2.0f * a));
		}

		if (t1 > 0.0f && t2 > 0.0f)
		{
			t = (std::min)(t1, t2);
		}
		else if (t1 > 0.0f)
		{
			t = t1;
		}
		else if (t2 > 0.0f)
		{
			t = t2;
		}
		else
		{
			t = -1;
		}
	}

	// 予測位置を出す
	DirectX::XMVECTOR TargetPos;
	if (t > 0.0f)
	{
		TargetPos = DirectX::XMVectorAdd(E, DirectX::XMVectorScale(V, t));
	}
	else
	{
		TargetPos = E;
	}

	DirectX::XMFLOAT3 result;
	DirectX::XMStoreFloat3(&result, TargetPos);
	return result;
}

// 偏差込みの発射方向(主に敵用)
DirectX::XMFLOAT3 LockOnSystem::GetAimDirection(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const
{
	if (!IsLocked()) return aimDirection;

	// 前半は関数を呼ぶだけ
	DirectX::XMFLOAT3 predicted = GetPredictedPosition(muzzlePos, projectileSpeed);

	DirectX::XMVECTOR TargetPos = DirectX::XMLoadFloat3(&predicted);
	DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&muzzlePos);
	DirectX::XMVECTOR Dir = DirectX::XMVectorSubtract(TargetPos, P);
	float length = DirectX::XMVectorGetX(DirectX::XMVector3Length(Dir));

	if (length < 0.001f) return aimDirection;

	DirectX::XMFLOAT3 aim;
	DirectX::XMStoreFloat3(&aim, DirectX::XMVector3Normalize(Dir));
	return aim;
}

float LockOnSystem::GetLockProgress() const
{
	if (lockTime <= 0.0f) return 1.0f; // 0以下なら1を返す
	return lockTimer / lockTime;
}

// エイムアシストをどのくらい効かせるか
float LockOnSystem::AimAssistScale()
{
	if (!target || assistAngle <= 0.0f) return 0.0f;

	Camera& camera = CameraManager::Instance().GetMainCamera();

	// カメラの正面とカメラ->敵からアシスト判定の角度を計算する
	DirectX::XMFLOAT3 aimPos = target->GetPosition();
	aimPos.y += 0.8f;

	// 画面外ならアシストしない
	if (!IsEnemyInScreen(aimPos, camera)) return 0.0f;

	DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&aimPos);
	DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat3(&camera.GetEye());
	DirectX::XMVECTOR Vec = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(targetPos, cameraPos));
	DirectX::XMVECTOR Front = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&camera.GetFront()));

	float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(Vec, Front)); // 内積を取る
	float angle = acosf(std::clamp(dot, -1.0f, 1.0f)); // Nanを防ぐ

	float rate = 1.0f - angle / DirectX::XMConvertToRadians(assistAngle);
	rate = std::clamp(rate, 0.0f, 1.0f);
	if (rate <= 0.0f) return 0.0f;

	return rate;
}

// 追尾型エイムアシスト
DirectX::XMFLOAT2 LockOnSystem::GetTarckingAssist(const DirectX::XMFLOAT3& origin, bool hasInput)
{
	float scale = AimAssistScale();
	// アシスト範囲外ならリセット
	if (scale <= 0.0f)
	{
		hasPrevAngle = false;
		return { 0, 0 };
	}

	DirectX::XMFLOAT3 aimPos = target->GetPosition();
	aimPos.y += 0.8f; // 敵の高さ

	float dx = aimPos.x - origin.x;
	float dy = aimPos.y - origin.y;
	float dz = aimPos.z - origin.z;
	float yaw = atan2f(dx, dz);
	float pitch = -atan2f(dy, sqrtf(dx * dx + dz * dz));

	// 初回だけ記録
	if (!hasPrevAngle)
	{
		prevTargetAngle = { pitch, yaw };
		hasPrevAngle = true;
		return { 0, 0 }; // 記録だけやから0を返す
	}

	float dPitch = pitch - prevTargetAngle.x;
	float dYaw = yaw - prevTargetAngle.y;

	// ヨーが±πを超えた時
	if (dYaw > DirectX::XM_PI) dYaw -= DirectX::XM_2PI;
	if (dYaw < -DirectX::XM_PI) dYaw += DirectX::XM_2PI;

	prevTargetAngle = { pitch, yaw };

	if (!hasInput) return { 0, 0 };
	float s = trackingStrength * scale;

	return { dPitch * s, dYaw * s };
}

// ロックオンできるか
bool LockOnSystem::IsTargetable(const DirectX::XMFLOAT3& aimDirection) const
{
	// 距離の計算
	DirectX::XMVECTOR ownerPos = DirectX::XMLoadFloat3(&owner->GetPosition());
	DirectX::XMVECTOR targetPos = DirectX::XMLoadFloat3(&target->GetPosition());
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(targetPos, ownerPos);
	float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(Vec));
	if (dist < 0.00001f) return false;

	if (!hardLock)
	{
		// 角度の計算
		DirectX::XMVECTOR ownerDirection = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&aimDirection));
		DirectX::XMVECTOR targetDirection = DirectX::XMVector3Normalize(Vec);
		float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(targetDirection, ownerDirection));

		// その値がlockAngle以下ならfalse
		if (dot < std::cosf(DirectX::XMConvertToRadians(lockAngle)))
		{
			return false;
		}
	}

	// 画面外ならfalse
	Camera& camera = CameraManager::Instance().GetMainCamera();
	if (!IsEnemyInScreen(target->GetPosition(), camera))
	{
		return false;
	}

	// プレイヤーとターゲットの位置がrange以上ならfalse
	if (dist > range)
	{
		return false;
	}
	
	return true;
}

// 敵が画面内にいるか
bool LockOnSystem::IsEnemyInScreen(const DirectX::XMFLOAT3& worldPos, const Camera& camera) const
{
	using namespace DirectX;

	// viewとprojectionを1つの行列にまとめる
	XMMATRIX View = XMLoadFloat4x4(&camera.GetView());
	XMMATRIX Projection = XMLoadFloat4x4(&camera.GetProjection());
	XMMATRIX ViewProjection = View * Projection;

	// w = 1 に変換する
	XMVECTOR Pos = XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);

	// ワールド→ビュー→クリップ座標
	XMVECTOR Clip = XMVector4Transform(Pos, ViewProjection);

	XMFLOAT4 clip;
	XMStoreFloat4(&clip, Clip);

	// カメラの後ろにある
	if (clip.w <= 0.0f) return false;

	// wで割ってNDC（-1～1）にする
	float ndcX = clip.x / clip.w;
	float ndcY = clip.y / clip.w;

	// 画面外ならfalse
	if (ndcX < -1.0f || ndcX > 1.0f ||
		ndcY < -1.0f || ndcY > 1.0f) 
	{
		return false;
	}

	return true;
}
