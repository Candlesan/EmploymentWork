#ifndef LIGHTS_HLSLI
#define LIGHTS_HLSLI

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS 8

struct PointLightData
{
    float4 position; // xyz:位置 w:range
    float4 color; // 色×強さ
};

struct SpotLightData
{
    float4 position; // xyz:位置 w:range
    float4 direction; // xyz:向き(正規化済み)
    float4 color; // 色×強さ
    float4 cone; // x:cos(インナー) y:cos(アウター)
};

cbuffer CbLight : register(b12)
{
    int pointLightCount;
    int spotLightCount;
    int2 lightpad;
    PointLightData pointLights[MAX_POINT_LIGHTS];
    SpotLightData spotLights[MAX_POINT_LIGHTS];
}

// 距離減衰：range でちょうど0。2乗で境目をなめらかにする
float CalcDistanceAttenuation(float dist, float range)
{
    float a = saturate(1.0f - dist / range);
    return a * a;
}

// 角度減衰：インナーの内側は1、アウターの外側は0
// L       : ライト→ピクセルの向き（正規化済み）
// spotDir : スポットの向き（正規化済み）
float CalcSpotAttenuation(float3 L, float3 spotDir, float cosInner, float cosOuter)
{
    float cosAngle = dot(L, spotDir);
    return saturate((cosAngle - cosOuter) / (cosInner - cosOuter));

}

#endif