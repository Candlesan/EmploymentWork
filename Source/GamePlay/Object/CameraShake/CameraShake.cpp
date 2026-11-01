#include "CameraShake.h"

#include "System/Core/Mathf.h"

#include <algorithm>
#include <imgui.h>


// ˆÊ’u‚ ‚è—h‚ê
void CameraShakeManager::Add(const DirectX::XMFLOAT3& pos, float radius, float strength, float duration)
{
	// ‹­‚³‚©ŠÔ‚ª0ˆÈ‰º‚È‚ç—h‚ç‚·ˆÓ–¡‚ª–³‚¢‚©‚ç“o˜^‚µ‚È‚¢
	if (strength <= 0.0f || duration <= 0.0f) return;

	CameraShake newShake;
	newShake.position = pos;
	newShake.radius = radius;
	newShake.strength = strength;
	newShake.duration = duration;
	newShake.timer = 0.0f;
	shakes.push_back(newShake);
}

// XVˆ—
void CameraShakeManager::Update(float elapsedTime)
{
	// ŠÔ‚ği‚ß‚é
	for (auto& s : shakes)
	{
		s.timer += elapsedTime;
	}

	// I‚í‚Á‚½—h‚ê‚ğÁ‚·
	shakes.erase(
		std::remove_if(shakes.begin(), shakes.end(),
			[](const CameraShake& s) {return s.timer >= s.duration; }),
		shakes.end());
}

// ¡ƒtƒŒ[ƒ€‚Ç‚ê‚¾‚¯—h‚ç‚·‚©
DirectX::XMFLOAT3 CameraShakeManager::GetOffset(const DirectX::XMFLOAT3& listenerPos)
{
	DirectX::XMVECTOR LPos = DirectX::XMLoadFloat3(&listenerPos);
	float maxPower = 0.0f;

	for (auto& s : shakes)
	{
		DirectX::XMVECTOR SPos = DirectX::XMLoadFloat3(&s.position);
		// —h‚êˆê‚Â‚¸‚ÂŒvZ
		float dist = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(LPos, SPos)));
		float distAtt = 1.0f - std::clamp(dist / s.radius, 0.0f, 1.0f);
		distAtt *= distAtt; // 2æ‚·‚é‚Æ’†S‚¾‚¯‹­‚­‚È‚é‰‰o‚ªì‚ê‚é

		// ŠÔ‚ÌŒ¸Š
		float timeAtt = 1.0f - s.timer / s.duration;
		timeAtt *= timeAtt;

		float power = s.strength * distAtt* timeAtt;

		// ˆê”Ô‹­‚¢—h‚ê‚ğÌ—p‚·‚é
		maxPower = (std::max)(maxPower, power);

		lastPower = maxPower;
	}

	// ‘å‚«‚³~ƒ‰ƒ“ƒ_ƒ€‚ÈŒü‚«(-1`1)
	return {
		Mathf::GetRandRange() * maxPower,
		Mathf::GetRandRange() * maxPower,
		Mathf::GetRandRange() * maxPower
	};
}

void CameraShakeManager::Clear()
{
	shakes.clear();
}

void CameraShakeManager::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	for (auto& s : shakes)
	{
		if (s.radius <= 0.0f) continue; // ˆÊ’u‚È‚µ‚Í•`‚©‚È‚¢
		renderer->DrawSphere(s.position, s.radius, { 1.0f, 0.5f, 0.0f, 1.0f });
	}
}

void CameraShakeManager::DrawGUI()
{
	ImGui::Text(u8"—h‚ê‚Ì” : %d", (int)shakes.size());
	ImGui::Text(u8"¡‚Ì‹­‚³ : %.3f", lastPower);
	for (auto& s : shakes)
	{
		ImGui::Text(u8"c‚è %.2f•b / ‹­‚³ %.2f / ”¼Œa %.1f", s.duration - s.timer, s.strength, s.radius);
	}
}