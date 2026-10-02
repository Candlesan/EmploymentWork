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
    if (fillMethod == 1) // ‰E‚©‚ç¶‚Ö
        t = local.x;
    else if (fillMethod == 2) // ‰E‚©‚ç¶‚Ö
        t = 1.0f - local.x;
    else if (fillMethod == 3) // ‰º‚©‚çã‚Ö
        t = 1.0f - local.y;
    else if (fillMethod == 4) // ã‚©‚ç‰º‚Ö
        t = local.y;
    else if (fillMethod == 5) // ŽžŒv‰ñ‚è
    {
        float2 p = local - 0.5f;
        float angle = atan2(p.x, -p.y);
        t = frac(angle / PI2 + 1.0f);
    }
    
        if (fillMethod != 0 && t >= fillAmount)
            discard;

    return spriteTexture.Sample(spriteSampler, pin.texcoord) * pin.color;
}