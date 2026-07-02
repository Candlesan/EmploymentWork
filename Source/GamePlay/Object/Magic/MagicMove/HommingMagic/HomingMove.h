#pragma once
#include "GamePlay/Object/Magic/IMagicMove.h"

class HomingMove : public IMagicMove
{
public:
	HomingMove(DirectX::XMFLOAT3 targetPos, float turnSpeed);

	void UpdateMove(
		DirectX::XMFLOAT3& position,
		DirectX::XMFLOAT3& direction,
		float speed,
		float elapsedTime)override;

private:
	DirectX::XMFLOAT3 target;
	float turnSpeed;
	bool isHoming = true;
};