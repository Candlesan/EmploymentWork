#pragma once
#include <DirectXMath.h>
#include <vector>
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"


class CameraShakeManager
{
public:
	struct CameraShake
	{
		DirectX::XMFLOAT3 position; // 発生位置
		float radius;				// 影響範囲（球）
		float strength;				// 最大の揺れ幅
		float duration;				// 揺れる秒数
		float timer;				// 経過時間
	};

public:
	// シングルトン
	static CameraShakeManager& Instance()
	{
		static CameraShakeManager instans;
		return instans;
	}

	// 位置あり揺れ
	void Add(const DirectX::XMFLOAT3& pos, float radius, float strength, float duration);

	// 位置無し揺れ
	void Add(float strength, float duration)
	{
		Add({ 0.0f, 0.0f, 0.0f }, -1.0f, strength, duration);
	}

	// 更新処理
	void Update(float elapsedTime);

	// 今フレームどれだけ揺らすか
	DirectX::XMFLOAT3 GetOffset(const DirectX::XMFLOAT3& listenerPos);

	void Clear();

	void RenderDebugPrimitive(ShapeRenderer* renderer);

	void DrawGUI();
private:
	CameraShakeManager() = default;
	~CameraShakeManager() = default;

private:
	std::vector<CameraShake> shakes;
	float lastPower = 0.0f;
};