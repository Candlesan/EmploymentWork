#include "SkyMap.hlsli"

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    //pin.world_positionにはピクセル毎のワールド座標が入ってくるので、
    //それを用いてピクセル毎の視線ベクトルを求める。
    float3 E = normalize(pin.world_position.xyz - cameraPosition.xyz);
    
    return SampleSkybox(texture0, sampler0, E);
}
