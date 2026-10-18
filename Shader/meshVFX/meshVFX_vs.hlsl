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
    vout.texcoord = texcoord;
    
    // リングのときだけ tangent を頂点カラーとして掛ける（モデルは 1,1,1,1 のまま）
    float4 vertexColor = lerp(float4(1, 1, 1, 1), tangent, useVertexColor);
    vout.color = vfxColor * vertexColor;
    
    return vout;
}