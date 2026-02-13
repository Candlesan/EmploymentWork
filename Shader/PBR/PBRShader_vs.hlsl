#include "PBRShader.hlsli"
#include "../Common/Skinning.hlsli"

VS_OUT main(
    float4 position : POSITION,
    float3 normal : NORMAL,
    float4 tangent : TANGENT,
    float2 texcoord : TEXCOORD,
    float4 boneWeights : BONE_WEIGHTS,
    uint4 boneIndices : BONE_INDICES)
{
    float sigma = tangent.w;
    
    VS_OUT vout = (VS_OUT) 0;
    
    // スキニング処理
    float4 skinnedPosition = SkinningPosition(position, boneWeights, boneIndices);
    float3 skinnedNormal = SkinningVector(normal, boneWeights, boneIndices);
    float3 skinnedTangent = SkinningVector(tangent.xyz, boneWeights, boneIndices);
    
    // スクリーン座標（スキニング後の座標を使用）
    vout.position = mul(skinnedPosition, viewProjection);
    
    // ワールド座標（スキニング後の座標をワールド変換）
    vout.w_position = skinnedPosition;
        
    // スキニング後の法線・タンジェントを使用
    vout.w_normal = normalize(skinnedNormal);
    vout.w_tangent = normalize(float4(skinnedTangent, sigma));
    
    vout.texcoord = texcoord;
    
     // シャドウマップ用のパラメーター計算
    //{
    //    float4 wvpPos = mul(position, light_view_projection);
    //    wvpPos.xyz /= wvpPos.w;
    //    wvpPos.y = -wvpPos.y; // Y軸を反転
    //    wvpPos.xy = 0.5f * wvpPos.xy + 0.5f;
    //    vout.shadow_texcoord = wvpPos.xyz;
    //}
    
    return vout;
}