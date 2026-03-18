#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>

//コリジョン
class Collision
{
public:
	//球と球の交差判定
	static bool IntersectSphereVsSpere(
		const DirectX::XMFLOAT3& positionA,
		float radiusA,
		const DirectX::XMFLOAT3& positionB,
		float radiusB,
		DirectX::XMFLOAT3& outPositionB
	);

	// 球とカプセルの交差判定
	static bool IntersectSphereVsCapsule(
		const DirectX::XMFLOAT3& spherePosition,
		float sqhereRadius,
		const DirectX::XMFLOAT3& capsulePosition,
		const DirectX::XMFLOAT3& capsuleDirection,
		float capsuleHeight,
		float capsuleRadius,
		DirectX::XMFLOAT3& outSphererPosition);

	// カプセルとカプセルの交差判定(重み有りバージョン)
	static bool IntersectCapsuleVsCapsule(
		const DirectX::XMFLOAT3& positionA,
		const DirectX::XMFLOAT3& directionA,
		float heightA,
		float radiusA,
		float weightA,
		const DirectX::XMFLOAT3& positionB,
		const DirectX::XMFLOAT3& directionB,
		float heightB,
		float radiusB,
		float weightB,
		DirectX::XMFLOAT3& outPositionA,
		DirectX::XMFLOAT3& outPositionB
	);
private:
	// ヘルパー関数：線分と線分の最短距離を求める計算
	static void ClosetPointSegmentSegment(
		const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& q1,
		const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& q2,
		DirectX::XMFLOAT3& c1, DirectX::XMFLOAT3& c2
	);
};