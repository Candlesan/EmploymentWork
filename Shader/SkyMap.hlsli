#include "Scene.hlsli"

struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float4 world_position : WORLD_POSITION;
};

cbuffer SKY_MAP_CONSTAN_TBUFFER : register(b8)
{
    row_major float4x4 inverse_view_projection;
}

#include "shading_functions.hlsli"