#include "PBRShader.hlsli"

// テクスチャ
Texture2D baseColorTexture : register(t0);
Texture2D emissiveTexture : register(t1);
Texture2D metalnessRoughnessTexture : register(t2);
Texture2D occlusionTexture : register(t3);
Texture2D normalTexture : register(t4);

// サンプラー
SamplerState samplerLinear : register(s0);
SamplerState sampler_anistoropic : register(s1);

// IBL用テクスチャ
TextureCube diffuse_iem : register(t33);
TextureCube specular_pmrem : register(t34);
Texture2D lut_ggx : register(t35);

// シャドウマップ
Texture2D stage_shadow_map : register(t5);
Texture2D character_shadow_map : register(t6);
SamplerComparisonState shadow_sampler_state : register(s5);


float SampleShadowPCF(Texture2D map, float3 tc, float bias)
{
    float2 size;
    map.GetDimensions(size.x, size.y);
    float2 texel = 1.0f / size;
    float depth = tc.z - bias;
    
    float lit = 0.0f;
    [unroll]for (int y = -1; y <= 1; ++y)
    [unroll]for (int x = -1; x <= 1; ++x)
            lit += map.SampleCmpLevelZero(shadow_sampler_state, tc.xy + float2(x, y) * texel, depth);
    
    return lit / 9.0f;
}


float4 main(VS_OUT pin, bool is_front_face : SV_IsFrontFace) : SV_TARGET
{
    // ベースカラーを取得
    float4 base_color = materialColor;
    if (hasBaseColorTexture > 0)
    {
        float4 tex_color = baseColorTexture.Sample(samplerLinear, pin.texcoord);
        tex_color.rgb = pow(tex_color.rgb, GammaFactor);
        base_color *= tex_color;
        clip(base_color.a - alphaCutoff);
    }
    
    // 自己発光色を取得
    float3 emisive_color = emissiveColor;
    if (hasEmissiveTexture > 0)
    {
        float3 emissive_tex = emissiveTexture.Sample(samplerLinear, pin.texcoord).rgb;
        emissive_tex = pow(emissive_tex, GammaFactor);
        emisive_color *= emissive_tex;
    }

    // 法線/従法線/接線の計算
    float3 N = normalize(pin.w_normal.xyz);
    float3 T = normalize(pin.w_tangent.xyz);
    float sigma = pin.w_tangent.w;
    T = normalize(T - N * dot(N, T));
    float3 B = normalize(cross(N, T) * sigma);
    
    // 裏面描画の場合は反転
    if (is_front_face == false)
    {
        T = -T;
        B = -B;
        N = -N;
    }

    // 法線マッピング
    if (hasNormalTexture > 0)
    {
        float3 normal_map = normalTexture.Sample(samplerLinear, pin.texcoord);
        normal_map = (normal_map * 2.0f) - 1.0f;
        normal_map = normalize(normal_map * float3(1.0f, 1.0f, 1.0f));
        N = normalize((normal_map.x * T) + (normal_map.y * B) + (normal_map.z * N));
    }

    // 金属質/粗さを取得
    float final_roughness = roughness;
    float final_metalness = metalness;
    if (hasMetalnessRoughnessTexture > 0)
    {
        float4 sampled = metalnessRoughnessTexture.Sample(samplerLinear, pin.texcoord);
        final_roughness *= sampled.g;
        final_metalness *= sampled.b;
    }

#if 1
    final_roughness = clamp(final_roughness + adjustroughness, 0.0001f, 1.0f);
    final_metalness = clamp(final_metalness + adjustmetalness, 0.0001f, 1.0f);
#endif

    // 光の遮蔽値を取得
    float occlusion_factor = 1.0f;
    if (hasOcclusionTexture > 0)
    {
        occlusion_factor = occlusionTexture.Sample(samplerLinear, pin.texcoord).r;
    }
    const float occlusion_strength = occlusionStrength;
    
    // (非金属部分)
    float4 albedo = base_color;
    
    // 入射光のうち拡散反射になる割合
    float3 diffuse_reflectance = lerp(albedo.rgb, 0.0f, final_metalness);
    
    // 垂直反射時のフレネル反射率
    float3 F0 = lerp(0.04f, albedo.rgb, final_metalness);
    
    // 視線ベクトル
    float3 V = normalize(pin.w_position.xyz - cameraPosition.xyz);

    // 直接光のシェーディング
    float3 total_diffuse = 0, total_specular = 0;
    {
        // 平行光源の処理
        {
            float3 diffuse = (float3) 0, specular = (float3) 0;
            float3 L = normalize(lightDirection.xyz);
            float3 LC = lightColor.rgb;
            DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, final_roughness, diffuse, specular);

            float stage_lit = SampleShadowPCF(stage_shadow_map, pin.stage_shadow_texcoord, stage_shadow_bias);
            float chara_lit = SampleShadowPCF(character_shadow_map, pin.character_shadow_texcoord, character_shadow_bias);
            
            float stage_atten = lerp(1.0f, stage_shadow_attenuation, 1.0f - stage_lit);
            float chara_atten = lerp(1.0f, character_shadow_attenuation, 1.0f - chara_lit);
            float atten = min(stage_atten, chara_atten); // 暗いほうを使う
            
            diffuse *= atten;
            specular *= atten;
            total_diffuse += diffuse;
            total_specular += specular;
        }
    }
    
    // IBL処理
    float3 ambient = ambient_color.rgb * ambient_color.a;
    total_diffuse += ambient * DiffuseIBL(N, V, final_roughness, diffuse_reflectance, F0, diffuse_iem, samplerLinear);
    total_specular += ambient * SpecularIBL(N, V, final_roughness, F0, lut_ggx, specular_pmrem, samplerLinear);

    // 遮蔽処理
    total_diffuse = lerp(total_diffuse, total_diffuse * occlusion_factor, occlusionStrength);
    total_specular = lerp(total_specular, total_specular * occlusion_factor, occlusionStrength);
      
    // 色生成
    float3 color = total_diffuse + total_specular + emisive_color;
    
    // sRGB空間へ
    color.rgb = pow(color.rgb, 1.0f / GammaFactor);
    return float4(color, base_color.a);
}