#include "Collision.h"

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

// カプセルとカプセルの交差判定
bool Collision::IntersectCapsuleVsCapsule(
    const DirectX::XMFLOAT3& positionA,
    const DirectX::XMFLOAT3& directionA,
    float heightA,
    float radiusA,
    const DirectX::XMFLOAT3& positionB,
    const DirectX::XMFLOAT3& directionB,
    float heightB,
    float radiusB,
    DirectX::XMFLOAT3& outPositionB)
{
    using namespace DirectX;

    // カプセルAの線分の端点を計算
    XMVECTOR PosA = XMLoadFloat3(&positionA);
    XMVECTOR DirA = XMLoadFloat3(&directionA);
    XMVECTOR HalfHeightA = XMVectorScale(DirA, heightA * 0.5f);

    XMFLOAT3 p1, q1;
    XMStoreFloat3(&p1, XMVectorSubtract(PosA, HalfHeightA));
    XMStoreFloat3(&q1, XMVectorAdd(PosA, HalfHeightA));

    // カプセルBの線分の端点を計算
    XMVECTOR PosB = XMLoadFloat3(&positionB);
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

        // カプセルBを押し出す
        XMVECTOR PushVec = XMVectorScale(pushDir, overlap);
        XMVECTOR NewPosB = XMVectorAdd(PosB, PushVec);
        XMStoreFloat3(&outPositionB, NewPosB);

        return true;
    }

    outPositionB = positionB;
    return false;
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