#include "../Common/Scene.hlsli"


struct VS_OUT
{
    float4 position : SV_Position;
    float4 w_position : POSITION;
    float3 w_normal : NORMAL;
    float4 w_tangent : TANGENT;
    float2 texcoord : TEXCOORD;
    //float3 shadow_texcoord : TEXCOORD1;
};

cbuffer MATERIAL_CONSTANT_BUFFER : register(b13)
{
    float4 materialColor;
    
    float3 emissiveColor;
    float pad0; // C++‘¤‚Ìpad0‚Æˆê’v‚³‚¹‚é

    float4 ambient_color;

    float metalness;
    float roughness;
    float adjustmetalness;
    float adjustroughness;

    float occlusionStrength;
    float alphaCutoff;
    int hasBaseColorTexture;
    int hasEmissiveTexture;

    int hasMetalnessRoughnessTexture;
    int hasOcclusionTexture;
    int hasNormalTexture;
    float pad1;
};
//cbuffer SHADOWMAP_CONSTANT_BUFFER : register(b5)
//{
//    row_major float4x4 light_view_projection;
//    float4 shadow_color;
//    float shadow_attenuation;
//    float shadow_bias;
//    float2 shadow_dummy;
//};

#include "../Common/shading_functions.hlsli"
#include "../Common/physical_based_rendering_functions.hlsli"
