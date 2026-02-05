#include "PBRShader.hlsli"
#include "Skinning.hlsli"

float4 main(
    float4 position : POSITION,
    float3 normal : NORMAL,
    float4 tangent : TANGENT,
    float2 texcoord : TEXCOORD,
    float4 boneWeights : BONE_WEIGHTS,
    uint4 boneIndices : BONE_INDICES) : SV_POSITION
{
    // スキニング処理
    float4 skinnedPosition = SkinningPosition(position, boneWeights, boneIndices);
    
    // ライトから見た座標系に変換
    return mul(skinnedPosition, light_view_projection);
}