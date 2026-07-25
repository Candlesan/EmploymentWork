#include "meshVFX.hlsli"

// 頂点シェーダーエントリポイント
VS_OUT main(
    float4 position : POSITION,
    float3 normal : NORMAL,
    float4 tangent : TANGENT,
    float2 texcoord : TEXCOORD)
{
    VS_OUT vout;
    vout.position = mul(position, worldViewProjection);
    vout.color = vfxColor;
    vout.texcoord = texcoord + uvOffset;

    return vout;
}