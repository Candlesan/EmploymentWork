#include "compute_particle.hlsli"

RWStructuredBuffer<particle_data> particle_data_buffer : register(u0); //  パーティクル管理バッファ
AppendStructuredBuffer<uint> particle_unused_buffer : register(u1); //  パーティクル番号管理バッファ(末尾への追加専用)
RWByteAddressBuffer indirect_data_buffer : register(u2); //  インダイレクト用バッファ
RWStructuredBuffer<particle_header> particle_header_buffer : register(u3); //  パーティクルヘッダー管理バッファ

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
        return;
    }

    // --- 位置 ---
    // 速度更新
    data.posVelocity.xyz += data.posAcceleration.xyz * elapsed_time;

    // 位置更新
    data.position.xyz += data.posVelocity.xyz * elapsed_time;
    
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


    ////  切り取り座標を算出    
    data.texcoord.xy += data.parameter.zw * elapsed_time;
        
    //  徐々に透明にしていく
    //data.color.a = saturate(data.parameter.y);
    
    //  深度ソート値算出
    header.depth = mul(float4(data.position.xyz, 1), viewProjection).w;

    //  更新情報を格納
    particle_header_buffer[header_index] = header;
    particle_data_buffer[data_index] = data;
}
