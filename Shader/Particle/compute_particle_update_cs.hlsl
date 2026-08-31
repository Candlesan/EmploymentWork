#include "compute_particle.hlsli"

RWStructuredBuffer<particle_data> particle_data_buffer : register(u0); //  パーティクル管理バッファ
AppendStructuredBuffer<uint> particle_unused_buffer : register(u1); //  パーティクル番号管理バッファ(末尾への追加専用)
RWByteAddressBuffer indirect_data_buffer : register(u2); //  インダイレクト用バッファ
RWStructuredBuffer<particle_header> particle_header_buffer : register(u3); //  パーティクルヘッダー管理バッファ

// 速度ID (0:低速3 ～ 3:等速 ～ 6:高速3) を曲線の傾きウェイトに変換
float SpeedToWeight(uint speed)
{
    switch (speed)
    {
        case 0: return 0.0f; // 低速3
        case 1: return 0.25f; // 低速2
        case 2: return 0.5f; // 低速1
        case 3: return 1.0f; // 等速
        case 4: return 1.5f; // 高速1
        case 5: return 2.0f; // 高速2
        case 6: return 3.0f; // 高速3
        default: return 1.0f;
    }
}

// 3次エルミート補間
float EaseCurve(float t, uint startSpeed, uint endSpeed)
{
    t = saturate(t); // 0.0 ~ 1.0 にクランプ

    if (startSpeed == 3 && endSpeed == 3)
    {
        return t; // 等速なら直線
    }

    float v0 = SpeedToWeight(startSpeed);
    float v1 = SpeedToWeight(endSpeed);

    float t2 = t * t;
    float t3 = t2 * t;

    // 公式: (-2t^3 + 3t^2) + v0*(t^3 - 2t^2 + t) + v1*(t^3 - t^2)
    float result = (-2.0f * t3 + 3.0f * t2)
                 + v0 * (t3 - 2.0f * t2 + t)
                 + v1 * (t3 - t2);

    return saturate(result);
}

[numthreads(NumParticleThread, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    uint header_index = dispatch_thread_id.x;

    particle_header header = particle_header_buffer[header_index];

    uint data_index = header.particle_index;

    //  有効フラグが立っているものだけ処理
    if (header.alive == 0)
        return;

    particle_data data = particle_data_buffer[data_index];
    
    //  経過時間分減少させる
    data.parameter.y -= elapsed_time;
    if (data.parameter.y < 0)
    {
        //  寿命が付きたら未使用リストに追加
        header.alive = 0;
        particle_unused_buffer.Append(data_index);

        //  ヘッダー情報更新
        particle_header_buffer[header_index] = header;

        //  死亡数をカウントする
        indirect_data_buffer.InterlockedAdd(IndirectArgumentsNumDeadParticle, 1);
        
        // 描画データ側もサイズとアルファを完全に0にして更新する
        data.renderScale = float4(0, 0, 0, 0);
        data.renderColor = float4(0, 0, 0, 0);
        particle_data_buffer[data_index] = data;
        
        return;
    }

    // --- 位置 ---
    // 速度更新
    data.posVelocity.xyz += data.posAcceleration.xyz * elapsed_time;

    // 位置更新
    data.position.xyz += data.posVelocity.xyz * elapsed_time;
    
    // 差分値を足す
    data.position.xyz += parent_delta;
    
    // --- 回転 ---
    // 角速度更新
    data.rotVelocity.xyz += data.rotAcceleration.xyz * elapsed_time;

    // 回転更新
    data.rotation.xyz += data.rotVelocity.xyz * elapsed_time;

    // 拡大
    // 拡大速度更新
    data.scaleVelocity.xyz += data.scaleAcceleration.xyz * elapsed_time;

    // 拡大更新
    data.scale.xyz += data.scaleVelocity.xyz * elapsed_time;
    
    data.scale.xyz = max(0.0f, data.scale.xyz);

    // UVスクロール    
    data.texcoord.xy += data.parameter.zw * elapsed_time;
        
    // フェードの計算
    uint packed = (uint) data.fade_param.w;
    uint inStartSpeed = (packed) & 0xF;      // フェードインの始点速度
    uint inEndSpeed =   (packed >> 4) & 0xF; // フェードインの終点速度
    uint outStartSpeed = (packed >> 8) & 0xF; // フェードアウトの終点速度
    uint outEndSpeed =   (packed >> 12) & 0xF; // フェードアウトの終点速度
    
    float maxLifeTime = data.fade_param.z; // 初期寿命
    float currentlife = data.parameter.y; // 残り寿命
    float activeTime = maxLifeTime - currentlife; // 経過時間
    
    float fadeInTime = data.fade_param.x;
    float fadeOutTime = data.fade_param.y;
    
    float alpha = 1.0f;
    
    if (fadeInTime > 0.0001f && activeTime <= fadeInTime)
    {
        float t = activeTime / fadeInTime;
        alpha = EaseCurve(t, inStartSpeed, inEndSpeed);
    }
    else if (fadeOutTime > 0.0001f && activeTime >= (maxLifeTime - fadeOutTime))
    {
        float fadeStartTime = maxLifeTime - fadeOutTime;
        float t = (activeTime - fadeStartTime) / fadeOutTime;
        alpha = 1.0f - EaseCurve(t, outStartSpeed, outEndSpeed);
    }
    
    // 単一イージングの計算
    if (data.scale_param.x >= 0.0f)
    {
        // パックの取り出し
        uint scalePacked = (uint) data.scale_param.x;
        uint scaleStartSpeed = (scalePacked) & 0xF;
        uint scaleEndSpeed = (scalePacked >> 4) & 0xF;
        float scaleMaxLifeTime = data.scale_param.y;
        
        // 進行度を計算する
        float t = activeTime / scaleMaxLifeTime;
        float easeT = EaseCurve(t, scaleStartSpeed, scaleEndSpeed);
        
        // 始点色から終点色へ補間
        float scaleT = lerp(data.scaleUniformStart, data.scaleUniformEnd, easeT); // スカラーで補間
        data.renderScale.x = data.scale.x * scaleT;
        data.renderScale.y = data.scale.y * scaleT;
        data.renderScale.z = data.scale.z * scaleT;
    }
    else
    {
        data.renderScale = data.scale;

    }
    
    // 色の計算
    float4 finalColor = data.start_color;
    if (data.color_param.x >= 0.0f)
    {
        // パックの取り出し
        uint colorPacked = (uint) data.color_param.x;
        uint colorStartSpeed = (colorPacked) & 0xF;
        uint colorEndSpeed = (colorPacked >> 4) & 0xF;
        float colorMaxLifeTime = data.color_param.y;
        
        // 進行度を計算する
        float t = activeTime / colorMaxLifeTime;
        float easeT = EaseCurve(t, colorStartSpeed, colorEndSpeed);

        finalColor = lerp(data.start_color, data.end_color, easeT);
    }
    
    
    //  徐々に透明にしていく
    finalColor.a *= saturate(alpha);
    data.renderColor = finalColor;
    
    //  深度ソート値算出
    header.depth = mul(float4(data.position.xyz, 1), viewProjection).w;

    //  更新情報を格納
    particle_header_buffer[header_index] = header;
    particle_data_buffer[data_index] = data;
}
