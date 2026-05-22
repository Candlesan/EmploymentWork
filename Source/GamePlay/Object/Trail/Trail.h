#pragma once
#include <DirectXMath.h>

class Trail
{
public:
	void Update(DirectX::XMFLOAT3 root, DirectX::XMFLOAT3 tip);

	void Render();
private:
	static const int MAX_POLYGON = 16;
	DirectX::XMFLOAT3 trailPositions[2][MAX_POLYGON];
	bool spline;
	DirectX::XMFLOAT4 color = { 1, 0, 0, 1 };
};