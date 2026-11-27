#include "../Sprite/Sprite.hlsli"

cbuffer UIFILL_CONSTANT_BUFFER : register(b9)
{
    float4 uvRect;
    float fillAmount;
    int fillMethod;
    float highlight; // 0～1(選択中かどうかフェードさせる)
    float time; // 経過時間
    float4 glowColor; // 光の色。rgb = 色 a = 強さ
    int useMask; // 要素があるだけ1
    float3 pad;
};

Texture2D spriteTexture : register(t0);
Texture2D maskTexture : register(t1);
SamplerState spriteSampler : register(s0);

static const float PI2 = 6.28318530;

float4 main(VS_OUT pin) : SV_TARGET
{
    float2 local = (pin.texcoord - uvRect.xy) / uvRect.zw;
    
    float t = 0.0f;
    if (fillMethod == 1) // 右から左へ
        t = local.x;
    else if (fillMethod == 2) // 右から左へ
        t = 1.0f - local.x;
    else if (fillMethod == 3) // 下から上へ
        t = 1.0f - local.y;
    else if (fillMethod == 4) // 上から下へ
        t = local.y;
    else if (fillMethod == 5) // 時計回り
    {
        float2 p = local - 0.5f; // 画像の中心へ移動
        float angle = atan2(p.x, -p.y); // (x,-y)と反転させることで12時の上方向からスタートする
        t = frac(angle / PI2 + 1.0f); // (-180°～ 180°)
    }
    else if (fillMethod == 6) // 反時計回り
    {
        float2 p = local - 0.5f;
        float angle = atan2(-p.x, -p.y);
        t = frac(angle / PI2 + 1.0f);
    }
    
        if (fillMethod != 0 && t >= fillAmount)
            discard;
    
    float4 col = spriteTexture.Sample(spriteSampler, pin.texcoord) * pin.color;
    
    // useMaskが１かつハイライトなら
    if (useMask && highlight > 0.0f)
    {
        float m = maskTexture.Sample(spriteSampler, pin.texcoord).r;

        // 脈打つ光
        float pulse = 0.6f + 0.4f * sin(time * 4.0f);

        // 流れる光：斜めの帯が 1.5 秒ごとに左から右へ通る
        float band = local.x + local.y * 0.3f; // 斜めにする
        float pos = frac(time / 1.5f) * 1.6f - 0.3f;
        float shine = saturate(1.0f - abs(band - pos) / 0.06f);

        float glow = m * (pulse + shine * 2.0f) * highlight * glowColor.a;
        col.rgb += glowColor.rgb * glow;
    }

    return col;
}