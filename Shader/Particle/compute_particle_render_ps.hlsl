#include "compute_particle.hlsli"

Texture2D color_map : register(t0);

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
SamplerState sampler_states[3] : register(s0);

float4 main(PS_IN pin) : SV_TARGET0
{
    float4 tex = color_map.Sample(sampler_states[ANISOTROPIC], pin.texcoord);
    
    if (dissolve_mode == 1) // 通常ディゾルブ
    {
        float mask = tex.r * tex.a; // 形の元(アルファが無いテクスチャならrが効く)
        float t = 1.0f - pin.color.a;
        float vis = smoothstep(t, t + dissolve_softness, mask); 
        
        return float4(tex.rgb * pin.color.rgb, vis); // 色はテクスチャのまま、透明度だけ削る
    }
    else if (dissolve_mode == 2) // トゥーン調
    {
        // フェードで下がるアルファを「削れ具合」として使う
        float t = 1.0f - pin.color.a;
        float s = dissolve_softness;
    
        float shape = smoothstep(t, t + s, tex.r); // 外形
        float core = smoothstep(t + 0.25f, t + 0.25 + s, tex.r); // 内側
    
        float3 shadow = pin.color.rgb * float3(0.7f, 0.85f, 1.0f); // 青みがかった色
        //return float4(lerp(shadow, pin.color.rgb, core), shape);
        return float4(lerp(pin.color.rgb, shadow, core), shape); // 影反転
    }
    
    return tex * pin.color;
}
