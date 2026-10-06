#include "../Sprite/Sprite.hlsli"

cbuffer UIFILL_CONSTANT_BUFFER : register(b9)
{
    float4 uvRect;
    float fillAmount;
    int fillMethod;
    float2 dummy;
};

Texture2D spriteTexture : register(t0);
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

    return spriteTexture.Sample(spriteSampler, pin.texcoord) * pin.color;
}