#include "StraightMove.h"

void StraightMove::UpdateMove(
	DirectX::XMFLOAT3& position,
	DirectX::XMFLOAT3& direction,
	float speed,
	float elapsedTime)
{
	float moveDist = speed * elapsedTime;
	position.x += direction.x * moveDist;
	position.y += direction.y * moveDist;
	position.z += direction.z * moveDist;
}
