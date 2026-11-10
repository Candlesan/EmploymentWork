#include "SkyMap.hlsli"

//TextureCube texture0 : register(t0);
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    //pin.world_positionにはピクセル毎のワールド座標が入ってくるので、
    //それを用いてピクセル毎の視線ベクトルを求める。
    float3 E = normalize(pin.world_position.xyz - cameraPosition.xyz);
    
    float3 color = SampleSkybox(texture0, sampler0, E).rgb * sky_intensity;
    color = pow(max(color, 0.0f), 1.0f / GammaFactor); // ガンマ処理
    
    return float4(color, 1.0f);
    //return texture0.Sample(sampler0, E);
}
