#pragma once
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>


// 当たり判定の形状
enum class HitBoxShape
{
	Sphere,		// 球
	Capsule,	// カプセル
	Cylinder,	// 円柱
};

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

	// ステージ用の当たり判定
	// カプセルが円柱の中にあるか。はみ出していたら押し戻した位置を返す関数
	static bool ContainCapsuleInCylinder(
		const DirectX::XMFLOAT3& capsulePos, float capsuleRadius, float capsuleHeight,
		const DirectX::XMFLOAT3& cylinderPos, float cylinderRadius, float cylinderHeight,
		DirectX::XMFLOAT3& outPos);

	// 三角形の定義
	struct Triangle
	{
		DirectX::XMFLOAT3 positions[3];
		DirectX::XMFLOAT3 normal;
	};
	// エリアの定義
	struct Area
	{
		DirectX::BoundingBox	boundingBox;
		std::vector<int>		triangleIndices;
	};

	// スフィアキャスト
	static bool SphereCast(
		const DirectX::XMFLOAT3& origin,
		const DirectX::XMFLOAT3& direction,
		float radius,
		float& distance,
		const std::vector<Triangle>& triangles,
		const std::vector<Area>& areas,
		DirectX::XMFLOAT3& hitPosition,
		DirectX::XMFLOAT3& hitNormal,
		bool spaceDivisionFlag = false);
	
	static bool RayCastVsCapsule(
		const DirectX::XMFLOAT3& start,
		const DirectX::XMFLOAT3& end,
		const DirectX::XMFLOAT3& capsulePos,
		const DirectX::XMFLOAT3& capsuleDir,
		float height, float radius,
		DirectX::XMFLOAT3& hitPosition,
		float& distance);

private:
	// ヘルパー関数：線分と線分の最短距離を求める計算
	static void ClosetPointSegmentSegment(
		const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& q1,
		const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& q2,
		DirectX::XMFLOAT3& c1, DirectX::XMFLOAT3& c2
	);
};