#include "compute_particle.hlsli"

float4 main(PS_IN pin) : SV_TARGET
{
    float d = abs(pin.texcoord.y * 2.0f - 1.0f); // 0:‘Ñ‚Ì’†S@1:‚Ó‚¿
    float glow = pow(saturate(1.0f - d), 2.0f); // ŠŠ‚ç‚©‚ÉÁ‚¦‚é‚Ó‚¿
    float core = smoothstep(0.35f, 0.0f, d); // ’†S‚¾‚¯”’‚¢c
    
    float3 rgb = pin.color.rgb * glow + core * 0.8f;
    float a = pin.color.a * saturate(glow + core);
	return float4(rgb, a);
}