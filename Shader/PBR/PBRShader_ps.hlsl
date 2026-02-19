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
Texture2D shadow_map : register(t5);
SamplerState shadow_sampler_state : register(s5);


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
        //float3 normal_factor = sampled.xyz;
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
        final_roughness *= 1.0 - sampled.a;
        final_metalness *= sampled.r;
    }

#if 0 // 確認用のコードなので本体は不要
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
    
    // 垂直反射時のフレネル反射率（非金属でも最低は4%は鏡面反射する）
    float3 F0 = lerp(0.04f, albedo.rgb, final_metalness);
    
    // 視線ベクトル
    float3 V = normalize(pin.w_position.xyz - cameraPosition.xyz);

    // 直接光のシェーディング
    float3 total_diffuse = 0, total_specular = 0;
    {
        // 並行光源の処理
        {
            float3 diffuse = (float3) 0, specular = (float3) 0;
            float3 L = normalize(lightDirection.xyz);
            float3 LC = lightColor.rgb/* * lightColor.a*/;
            DirectBRDF(diffuse_reflectance, F0, N, V, L, LC, final_roughness, diffuse, specular);

            // 平行光源用シャドウマップ
            float depth = shadow_map.Sample(shadow_sampler_state, pin.shadow_texcoord.xy).r;
            //深度値を比較して壁かどうかを判定する
            
            if (pin.shadow_texcoord.z - depth > shadow_bias)
            {
                diffuse *= shadow_attenuation;
                specular *= shadow_attenuation;
            }
            
            total_diffuse += diffuse;
            total_specular += specular;
        }
    }
    
    // IBL処理
    float3 ambient = ambient_color.rgb * ambient_color.a;
    total_diffuse += ambient * DiffuseIBL(N, V, roughness, diffuse_reflectance, F0, diffuse_iem, samplerLinear);
    total_specular += ambient * SpecularIBL(N, V, roughness, F0, lut_ggx, specular_pmrem, samplerLinear);

    // 遮蔽処理
    total_diffuse = lerp(total_diffuse, total_diffuse * occlusion_factor, occlusionStrength);
    total_specular = lerp(total_specular, total_specular * occlusion_factor, occlusionStrength);
      
    // 色生成
    float3 color = total_diffuse + total_specular + emisive_color;
    
    // sRGB空間へ
    color.rgb = pow(color.rgb, 1.0f / GammaFactor);
    return float4(color, base_color.a);
}