#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMFLOAT3	direction = { 0, -1, 0 }; // 光の向き
	DirectX::XMFLOAT3	color = { 1, 1, 1 }; // 光の色
	float intensity = 1.0f; // 光の強さ
};

class LightManager
{
public:
	// ディレクショナルライト設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }

	// ディレクショナルライト取得
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }
	DirectionalLight& GetDirectionalLight() { return directionalLight; }

private:
	DirectionalLight	directionalLight;
};
