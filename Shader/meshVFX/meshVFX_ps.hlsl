#include "meshVFX.hlsli"

Texture2D spriteTexture : register(t0);
Texture2D noiseTexture : register(t1);
SamplerState spriteSampler : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    // ノイズ画像から色を取得
    float2 noiseUV = pin.texcoord + noiseUvOffset;
    float4 noiseColor = noiseTexture.Sample(spriteSampler, noiseUV);
    
    // ノイズのRG値を-0.5～+0.5 の範囲にして、歪みベクトルを作る
    float2 distortion = (noiseColor.rg - 0.5) * distortionPower;
    
    // メインテクスチャの座標をずらす
    float2 finalUV = pin.texcoord + uvOffset + distortion;
    
	// テクスチャ空画像の色を取得
    float4 texColor = spriteTexture.Sample(spriteSampler, finalUV);
	
    return texColor * vfxColor;
}