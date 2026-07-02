#pragma once
#include "GamePlay/Object/Magic/IMagicMove.h"

// ‚Ü‚Á‚·‚®”ò‚Î‚·‹…
class StraightMove : public IMagicMove
{
public:
	void UpdateMove(
		DirectX::XMFLOAT3& position,
		DirectX::XMFLOAT3& direction,
		float speed,
		float elapsedTime)override;
};
