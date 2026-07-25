#include "../Common/Scene.hlsli"

// 頂点シェーダー出力データ
struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color    : COLOR;
    float2 texcoord : TEXCOORD;
};

// 定数バッファ
cbuffer MeshVfxBuffer : register(b0)
{
    row_major float4x4 worldViewProjection; // WVP
    float4 vfxColor;                        // エフェクトのメインカラー
    float2 uvOffset;                        // UVスクロール用オフセット(X,Y)
    float2 noiseUvOffset;                   // ノイズ用オフセット
    float distortionPower;                 // 歪みの強さ
    float2 padding; 
}