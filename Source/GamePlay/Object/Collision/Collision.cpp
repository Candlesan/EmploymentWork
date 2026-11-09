#include "Collision.h"
#include <SphereCast.h>
#include <unordered_set>
#include <Windows.h> 
#include <string>


//球と球の交差判定
bool Collision::IntersectSphereVsSpere(
    const DirectX::XMFLOAT3& positionA,
    float radiusA,
    const DirectX::XMFLOAT3& positionB,
    float radiusB,
    DirectX::XMFLOAT3& outPositionB)
{
    //A→Bの単位ベクトルを算出
    DirectX::XMVECTOR PositionA = DirectX::XMLoadFloat3(&positionA);
    DirectX::XMVECTOR PositionB = DirectX::XMLoadFloat3(&positionB);
    DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PositionB, PositionA);
    DirectX::XMVECTOR LenfthSq = DirectX::XMVector3LengthSq(Vec);
    float lengthSq;
    DirectX::XMStoreFloat(&lengthSq, LenfthSq);

    //距離判定
    float range = (radiusA + radiusB) * (radiusA + radiusB);
    if (lengthSq > range)
    {
        return false;
    }

    //AがBを押し出す
    DirectX::XMVECTOR NormalizeVec = DirectX::XMVector3Normalize(Vec);
    DirectX::XMVECTOR pushback = DirectX::XMVectorScale(NormalizeVec, radiusA + radiusB);
    DirectX::XMVECTOR newPosB = DirectX::XMVectorAdd(PositionA, pushback);
    DirectX::XMStoreFloat3(&outPositionB, newPosB);
    return true;
}

// 球とカプセルの交差判定
bool Collision::IntersectSphereVsCapsule(
    const DirectX::XMFLOAT3& spherePosition,
    float sphereRadius,
    const DirectX::XMFLOAT3& capsulePosition,
    const DirectX::XMFLOAT3& capsuleDirection,
    float capsuleHeight,
    float capsuleRadius,
    DirectX::XMFLOAT3& outSphererPosition)
{
    using namespace DirectX;

    // カプセルの線分の端点を計算する
    XMVECTOR CapsulePos = DirectX::XMLoadFloat3(&capsulePosition);
    XMVECTOR CapsuleDir = DirectX::XMLoadFloat3(&capsuleDirection);

    // このpositionはカプセルの中心だから
    // 半分にして計算するのは中心から上下の伸ばすと合計でhightになるから
    // そのまま計算すると上端から線分が始まってカプセル全体が下にずれるから
    XMVECTOR HalfHeight = XMVectorScale(CapsuleDir, capsuleHeight * 0.5f);

    XMFLOAT3 p, q;
    XMStoreFloat3(&p, XMVectorSubtract(CapsulePos, HalfHeight));
    XMStoreFloat3(&q, XMVectorAdd(CapsulePos, HalfHeight));

    // 球の中心から線分への最近点を求める
    // 球は点として扱う
    XMFLOAT3 point, dummy;
    ClosetPointSegmentSegment(spherePosition, spherePosition, p, q, dummy, point);

    // 最近接点と球と中心との距離を計算
    XMVECTOR SpherePos = XMLoadFloat3(&spherePosition);
    XMVECTOR Point = XMLoadFloat3(&point);
    XMVECTOR Vec = XMVectorSubtract(SpherePos, Point);
    float dist = XMVectorGetX(XMVector3Length(Vec));

    float radius = sphereRadius + capsuleRadius;
    if (dist < radius)
    {
        // 球をカプセルから押し出す
        XMVECTOR pushDir;
        if (dist > 1e-6f)
        {
            pushDir = XMVector3Normalize(Vec);
        }
        else
        {
            pushDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        }

        float overlap = radius - dist;
        XMVECTOR newSpherePos = XMVectorAdd(SpherePos, XMVectorScale(pushDir, overlap));
        XMStoreFloat3(&outSphererPosition, newSpherePos);
        return true;
    }

    outSphererPosition = spherePosition;
    return false;
}

// カプセルとカプセルの交差判定
bool Collision::IntersectCapsuleVsCapsule(
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
    DirectX::XMFLOAT3& outPositionB)
{
    using namespace DirectX;

    // カプセルAの線分の端点を計算
    XMVECTOR PosA = XMLoadFloat3(&positionA);

    //// カプセルの「下端」が足元にくるように、中心を「高さの半分 + 半径」分だけ上にずらす
    //XMVECTOR OffsetA = XMVectorSet(0.0f, heightA * 0.5f + radiusA, 0.0f, 0.0f);
    //PosA = XMVectorAdd(PosA, OffsetA);

    XMVECTOR DirA = XMLoadFloat3(&directionA);
    XMVECTOR HalfHeightA = XMVectorScale(DirA, heightA * 0.5f);

    XMFLOAT3 p1, q1;
    XMStoreFloat3(&p1, XMVectorSubtract(PosA, HalfHeightA));
    XMStoreFloat3(&q1, XMVectorAdd(PosA, HalfHeightA));

    // カプセルBの線分の端点を計算
    XMVECTOR PosB = XMLoadFloat3(&positionB);

    //XMVECTOR OffsetB = XMVectorSet(0.0f, heightB * 0.5f + radiusB, 0.0f, 0.0f);
    //XMVECTOR CalcPosB = XMVectorAdd(PosB, OffsetB);

    XMVECTOR DirB = XMLoadFloat3(&directionB);
    XMVECTOR HalfHeightB = XMVectorScale(DirB, heightB * 0.5f);

    XMFLOAT3 p2, q2;
    XMStoreFloat3(&p2, XMVectorSubtract(PosB, HalfHeightB));
    XMStoreFloat3(&q2, XMVectorAdd(PosB, HalfHeightB));

    // 線分間の最近接点を計算
    XMFLOAT3 c1, c2;
    ClosetPointSegmentSegment(p1, q1, p2, q2, c1, c2);

    // 最近接点間の距離を計算
    XMVECTOR C1 = XMLoadFloat3(&c1);
    XMVECTOR C2 = XMLoadFloat3(&c2);
    XMVECTOR Vec = XMVectorSubtract(C2, C1);
    float dist = XMVectorGetX(XMVector3Length(Vec));

    // 半径の合計よりも距離が小さければ衝突
    float radiusSum = radiusA + radiusB;
    if (dist < radiusSum)
    {
        // 押し出しベクトルを計算
        float overlap = radiusSum - dist;
        XMVECTOR pushDir;

        if (dist > 1e-6f)
        {
            pushDir = XMVector3Normalize(Vec);
        }
        else
        {
            // 距離がほぼ0の場合は上方向に押し出す
            pushDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
        }

        // 2つのカプセルの重さから押し出し比率を求める
        float totalWeight = weightA + weightB;
        float raiotA = weightB / totalWeight;
        float raiotB = weightA / totalWeight;

        // 座標を更新
        XMVECTOR pushVecA = XMVectorScale(pushDir, -overlap * raiotA);
        XMVECTOR pushVecB = XMVectorScale(pushDir, overlap * raiotB);

        //// 調整済み中心座標から足元座標に戻す
        //XMVECTOR newPosA = XMVectorSubtract(XMVectorAdd(PosA, pushVecA), OffsetA);
        //XMVECTOR newPosB = XMVectorSubtract(XMVectorAdd(PosB, pushVecB), OffsetB);

        XMStoreFloat3(&outPositionA, XMVectorAdd(PosA, pushVecA));
        XMStoreFloat3(&outPositionB, XMVectorAdd(PosB, pushVecB));

        return true;
    }

    outPositionA = positionA;
    outPositionB = positionB;
    return false;
}

// カプセルと円柱の中にいるか。はみ出していたら押し戻した位置を返す
bool Collision::ContainCapsuleInCylinder(
    const DirectX::XMFLOAT3& capsulePos, float capsuleRadius, float capsuleHeight,
    const DirectX::XMFLOAT3& cylinderPos, float cylinderRadius, float cylinderHeight,
    DirectX::XMFLOAT3& outPos)
{
    outPos = capsulePos;
    bool isOut = false;

    // 横の計算
    float dx = capsulePos.x - cylinderPos.x;
    float dz = capsulePos.z - cylinderPos.z;
    float distXZ = sqrtf(dx * dx + dz * dz); // 円柱の中心からキャラの水平距離
    float limit = cylinderRadius - capsuleRadius; // キャラクターの動ける範囲

    if (distXZ > limit)
    {
        // 中心から今の方向に向かってlimitの位置まで戻す
        // cylinderPos +: 基準位置からの加算
        // dx / distXZ: 正規化
        // * limit: 限界距離まで引き延ばす
        outPos.x = cylinderPos.x + dx / distXZ * limit;
        outPos.z = cylinderPos.z + dz / distXZ * limit;
        isOut = true;
    }

    // 縦の計算
    float minY = cylinderPos.y; // 床
    float maxY = cylinderPos.y + cylinderHeight - capsuleHeight; // 天井から身長分下げた位置

    if (outPos.y < minY) { outPos.y = minY; isOut = true; }
    if (outPos.y > maxY) { outPos.y = maxY; isOut = true; }

    return isOut;
}

// スフィアキャスト
bool Collision::SphereCast(
    const DirectX::XMFLOAT3& origin,
    const DirectX::XMFLOAT3& direction,
    float radius,
    float& distance,
    const std::vector<Triangle>& triangles,
    const std::vector<Area>& areas,
    DirectX::XMFLOAT3& hitPosition,
    DirectX::XMFLOAT3& hitNormal,
    bool spaceDivisionFlag)
{
    static int callCount = 0;
    callCount++;

    bool hit = false;
    DirectX::XMVECTOR Start = DirectX::XMLoadFloat3(&origin);
    DirectX::XMVECTOR Direction = DirectX::XMLoadFloat3(&direction);

    // 終点 = 始点 ＋ (方向 × 距離)
    DirectX::XMVECTOR End = DirectX::XMVectorAdd(Start, DirectX::XMVectorScale(Direction, distance));

    // 空間分割せずに、普通にレイキャストする
    if (!spaceDivisionFlag)
    {
        // 三角形のリストと接触部分を総当たりで調べる
        for (const auto& triangle : triangles)
        {
            // 接触部分の位置を読み込む
            DirectX::XMVECTOR Positions[3] =
            {
                DirectX::XMLoadFloat3(&triangle.positions[0]),
                DirectX::XMLoadFloat3(&triangle.positions[1]),
                DirectX::XMLoadFloat3(&triangle.positions[2])
            };

            // 結果を取得する
            SphereCastResult result;
            if (IntersectSphereCastVsTriangle(Start, End, radius, Positions, &result))
            {
                if (distance < result.distance) continue;
                distance = result.distance;
                DirectX::XMVECTOR HitPosition = DirectX::XMVectorAdd(Start, DirectX::XMVectorScale(Direction, distance));
                DirectX::XMStoreFloat3(&hitPosition, HitPosition);
                DirectX::XMStoreFloat3(&hitNormal, result.normal);
                hit = true;
            }
        }
    }
    else // 空間分割したデータを使い、スフィアレイキャストを高速処理する
    {
        // レイのAABBを作る
        DirectX::BoundingBox rayAABB;
        DirectX::BoundingBox::CreateFromPoints(rayAABB, Start, End);

        rayAABB.Extents.x += radius;
        rayAABB.Extents.y += radius;
        rayAABB.Extents.z += radius;

        // キャスト全体を囲む箱の最小点と最大点
        const DirectX::XMFLOAT3 rayMin = {
            rayAABB.Center.x - rayAABB.Extents.x,
            rayAABB.Center.y - rayAABB.Extents.y,
            rayAABB.Center.z - rayAABB.Extents.z,
        };
        const DirectX::XMFLOAT3 rayMax = {
            rayAABB.Center.x + rayAABB.Extents.x,
            rayAABB.Center.y + rayAABB.Extents.y,
            rayAABB.Center.z + rayAABB.Extents.z,
        };

        // エリアごとにチェック
        for (const Area& area : areas)
        {
            // レイをAABBと交差してるエリアだけを見る
            if (!area.boundingBox.Intersects(rayAABB)) continue;

            // エリア内の三角形にレイキャスト
            for (int triIndex : area.triangleIndices)
            {
                const Triangle& triangle = triangles[triIndex];

                // 箱が重ならない三角形は重い判定をせずに飛ばす
                if (triangle.boundsMax.x < rayMin.x || triangle.boundsMin.x > rayMax.x ||
                    triangle.boundsMax.y < rayMin.y || triangle.boundsMin.y > rayMax.y ||
                    triangle.boundsMax.z < rayMin.z || triangle.boundsMin.z > rayMax.z)
                {
                    continue;
                }

                // 接触部分の位置を読み込む
                DirectX::XMVECTOR Positions[3] =
                {
                    DirectX::XMLoadFloat3(&triangle.positions[0]),
                    DirectX::XMLoadFloat3(&triangle.positions[1]),
                    DirectX::XMLoadFloat3(&triangle.positions[2])
                };

                // 結果を取得する
                SphereCastResult result;
                if (IntersectSphereCastVsTriangle(Start, End, radius, Positions, &result))
                {
                    if (distance < result.distance) continue;
                    distance = result.distance;
                    DirectX::XMVECTOR HitPosition = DirectX::XMVectorAdd(Start, DirectX::XMVectorScale(Direction, distance));
                    DirectX::XMStoreFloat3(&hitPosition, HitPosition);
                    DirectX::XMStoreFloat3(&hitNormal, result.normal);
                    hit = true;
                }
            }
        }
    }

    // 結果を返す
    return hit;
}

// ヘルパー関数：線分と線分の最短距離を求める計算
void Collision::ClosetPointSegmentSegment(
    const DirectX::XMFLOAT3& p1, const DirectX::XMFLOAT3& q1,
    const DirectX::XMFLOAT3& p2, const DirectX::XMFLOAT3& q2,
    DirectX::XMFLOAT3& c1, DirectX::XMFLOAT3& c2)
{
    using namespace DirectX;

    XMVECTOR P1 = XMLoadFloat3(&p1);
    XMVECTOR Q1 = XMLoadFloat3(&q1);
    XMVECTOR P2 = XMLoadFloat3(&p2);
    // XMVECTOR Q2 = XMLoadFloat3(&q2); // Q2 は使わないのでコメントアウト

    XMVECTOR d1 = XMVectorSubtract(Q1, P1); // 線分1の方向ベクトル
    XMVECTOR d2 = XMVectorSubtract(XMLoadFloat3(&q2), P2); // 線分2の方向ベクトル
    XMVECTOR r = XMVectorSubtract(P1, P2);

    float a = XMVectorGetX(XMVector3Dot(d1, d1)); // a: 線分1の長さの2乗
    float e = XMVectorGetX(XMVector3Dot(d2, d2)); // e: 線分2の長さの2乗
    float f = XMVectorGetX(XMVector3Dot(d2, r));  // f: d2 と r の内積
    float c = XMVectorGetX(XMVector3Dot(d1, r));  // c: d1 と r の内積
    float b = XMVectorGetX(XMVector3Dot(d1, d2)); // b: d1 と d2 の内積

    float s = 0.0f, t = 0.0f;
    const float epsilon = 1e-6f;

    // 線分1も線分2も点の場合 (P1, P2)
    if (a <= epsilon && e <= epsilon)
    {
        s = 0.0f;
        t = 0.0f;
    }
    // 線分1が点の場合 (P1)
    else if (a <= epsilon)
    {
        s = 0.0f;
        // 線分2 (P2->Q2) 上の最近接点 t をクランプ
        t = f / e;
        t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
    }
    // 線分2が点の場合 (P2)
    else if (e <= epsilon)
    {
        t = 0.0f;
        // 線分1 (P1->Q1) 上の最近接点 s をクランプ
        s = -c / a;
        s = (s < 0.0f) ? 0.0f : (s > 1.0f) ? 1.0f : s;
    }
    // 両方線分の場合
    else
    {
        float denom = a * e - b * b;

        if (denom != 0.0f)
        {
            s = (b * f - c * e) / denom;
            s = (s < 0.0f) ? 0.0f : (s > 1.0f) ? 1.0f : s; // s を [0, 1] にクランプ

            t = (b * s + f) / e;

            if (t < 0.0f)
            {
                t = 0.0f;
                // t=0 の時の s を再計算し、[0, 1] にクランプ
                s = -c / a;
                s = (s < 0.0f) ? 0.0f : (s > 1.0f) ? 1.0f : s;
            }
            else if (t > 1.0f)
            {
                t = 1.0f;
                // t=1 の時の s を再計算し、[0, 1] にクランプ
                s = (b - c) / a;
                s = (s < 0.0f) ? 0.0f : (s > 1.0f) ? 1.0f : s;
            }
        }
        else // 平行の場合 (denom = 0)
        {
            // ひとまず s=0, t=0 とするか、s を [0,1] にクランプして t を計算する
            // より正確には線分1の始点P1から線分2への最近接点を求める。
            // ここでは簡易的に s=0, t をクランプするロジックを採用
            s = 0.0f;
            t = f / e;
            t = (t < 0.0f) ? 0.0f : (t > 1.0f) ? 1.0f : t;
        }
    }

    XMVECTOR C1 = XMVectorAdd(P1, XMVectorScale(d1, s));
    XMVECTOR C2 = XMVectorAdd(P2, XMVectorScale(d2, t));

    XMStoreFloat3(&c1, C1);
    XMStoreFloat3(&c2, C2);
}

// カプセルとレイキャストの当たり判定
bool Collision::RayCastVsCapsule(
    const DirectX::XMFLOAT3& start,
    const DirectX::XMFLOAT3& end,
    const DirectX::XMFLOAT3& capsulePos,
    const DirectX::XMFLOAT3& capsuleDir,
    float height, float radius,
    DirectX::XMFLOAT3& hitPosition,
    float& distance)
{
    using namespace DirectX;

    // カプセルの軸を作る
    XMVECTOR CapsulePos = XMLoadFloat3(&capsulePos);
    XMVECTOR CapsuleDir = XMLoadFloat3(&capsuleDir);

    XMVECTOR HalfHeight = XMVectorScale(CapsuleDir, height * 0.5f);

    XMFLOAT3 p, q;
    XMStoreFloat3(&p, XMVectorSubtract(CapsulePos, HalfHeight));
    XMStoreFloat3(&q, XMVectorAdd(CapsulePos, HalfHeight));

    XMFLOAT3 c1, c2;
    ClosetPointSegmentSegment(start, end, p, q, c1, c2);

    XMVECTOR C1 = XMLoadFloat3(&c1);
    XMVECTOR C2 = XMLoadFloat3(&c2);
    XMVECTOR Vec = XMVectorSubtract(C2, C1);
    float Distance = XMVectorGetX(XMVector3LengthSq(Vec));

    if (Distance > radius * radius) return false;

    XMVECTOR Start = XMLoadFloat3(&start);
    XMVECTOR Vec2 = XMVectorSubtract(C1, Start);
    float dist = XMVectorGetX(XMVector3Length(Vec2));

    hitPosition = c1;
    distance = dist;

    return true;
}