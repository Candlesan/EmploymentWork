#include "LockOnSystem.h"
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
		ImGui::DragFloat(u8"ターゲット補足時間", &lockTime, 0.1f);
		ImGui::Separator();
		ui.DrawGUI();
	}
}

DirectX::XMFLOAT3 LockOnSystem::GetPredictedPosition(const DirectX::XMFLOAT3& muzzlePos, float projectileSpeed) const
{
	DirectX::XMVECTOR E = DirectX::XMLoadFloat3(&target->GetPosition());
	E = DirectX::XMVectorAdd(E, DirectX::XMVectorSet(0, 0.25f, 0, 0));
	DirectX::XMVECTOR V = DirectX::XMLoadFloat3(&target->GetVelocity());
	DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&muzzlePos);
	DirectX::XMVECTOR D = DirectX::XMVectorSubtract(E, P);

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

// 偏差込みのエイム
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

	// プレイヤーとターゲットの位置がrange以上ならfalse
	if (dist > range)
	{
		return false;
	}
	
	return true;
}
