#include "compute_particle.hlsli"

RWStructuredBuffer<particle_data> particle_data_buffer : register(u0); //  パーティクル管理バッファ
AppendStructuredBuffer<uint> particle_unused_buffer : register(u1); //  パーティクル番号管理バッファ(末尾への追加専用)
RWByteAddressBuffer indirect_data_buffer : register(u2); //  インダイレクト用バッファ
RWStructuredBuffer<particle_header> particle_header_buffer : register(u3); //  パーティクルヘッダー管理バッファ

[numthreads(NumParticleThread, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    //uint index = dispatch_thread_id.x;

    ////  有効フラグが立っているものだけ処理
    //if (particle_data_buffer[index].parameter.z < 0.0f)
    //    return;

    ////  経過時間分減少させる
    //particle_data_buffer[index].parameter.y -= elapsed_time;
    //if (particle_data_buffer[index].parameter.y < 0)
    //{
    //    //  寿命が付きたら未使用リストに追加
    //    particle_data_buffer[index].parameter.z = -1.0f; //  生存フラグを初期化しておく
    //    particle_unused_buffer.Append(index);
        
    //    //  死亡数をカウントする
    //    indirect_data_buffer.InterlockedAdd(IndirectArgumentsNumDeadParticle, 1);
    //    return;
    //}

    ////  速度更新
    //particle_data_buffer[index].velocity.xyz += particle_data_buffer[index].acceleration.xyz * elapsed_time;

    ////  位置更新
    //particle_data_buffer[index].position.xyz += particle_data_buffer[index].velocity.xyz * elapsed_time;

    ////  切り取り座標を算出
    //uint type = (uint) (particle_data_buffer[index].parameter.x + 0.5f);
    //float w = 1.0 / texture_split_count.x;
    //float h = 1.0 / texture_split_count.y;
    //float2 uv = float2((type % texture_split_count.x) * w, (type / texture_split_count.x) * h);
    //particle_data_buffer[index].texcoord.xy = uv;
    //particle_data_buffer[index].texcoord.zw = float2(w, h);
        
    ////  徐々に透明にしていく
    //particle_data_buffer[index].color.a = saturate(particle_data_buffer[index].parameter.y);
    
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

    //  速度更新
    data.velocity.xyz += data.acceleration.xyz * elapsed_time;

    //  位置更新
    data.position.xyz += data.velocity.xyz * elapsed_time;

    //  切り取り座標を算出
    uint type = (uint) (data.parameter.x + 0.5f);
    float w = 1.0 / texture_split_count.x;
    float h = 1.0 / texture_split_count.y;
    float2 uv = float2((type % texture_split_count.x) * w, (type / texture_split_count.x) * h);
    data.texcoord.xy = uv;
    data.texcoord.zw = float2(w, h);
        
    //  徐々に透明にしていく
    data.color.a = saturate(data.parameter.y);
    
    //  深度ソート値算出
    header.depth = mul(float4(data.position.xyz, 1), viewProjection).w;

    //  更新情報を格納
    particle_header_buffer[header_index] = header;
    particle_data_buffer[data_index] = data;
}
