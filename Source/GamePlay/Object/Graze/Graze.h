#pragma once
#include <DirectXMath.h>

class Graze
{
public:
    bool IsGraze(
        const DirectX::XMFLOAT3& characterPos,
        float characterYaw,
        bool isDefense,
        const DirectX::XMFLOAT3& magicPos,
        float magicRadius) const;

private:
	float radius = 0.5f; // ƒOƒŒƒCƒY‚Ì”ÍˆÍ
};