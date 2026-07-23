#include "compute_particle.hlsli"

RWStructuredBuffer<particle_data> particle_data_buffer : register(u0); //  パーティクル管理バッファ
ConsumeStructuredBuffer<uint> parcile_pool_buffer : register(u1); //  パーティクル番号管理バッファ(末尾から取出専用)

StructuredBuffer<emit_particle_data> emit_particle_butter : register(t0); //  パーティクル生成情報バッファ
RWByteAddressBuffer indirect_data_buffer : register(u2); //  インダイレクト用バッファ

RWStructuredBuffer<particle_header> particle_header_buffer : register(u3); //  パーティクルヘッダー管理バッファ

[numthreads(1, 1, 1)]
void main(uint3 dispatch_thread_id : SV_DispatchThreadID)
{
    //  未使用リストの末尾から未使用パーティクルのインデックスを取得
    uint particle_index = parcile_pool_buffer.Consume();
    uint emit_index = dispatch_thread_id.x;

    ////  エミッターの末端から番号取得
    //indirect_data_buffer.InterlockedAdd(IndirectArgumentsNumEmitParticleIndex, 1, particle_index);

    ////  パーティクル生成処理
    //particle_data_buffer[particle_index].parameter.x = emit_particle_butter[emit_index].parameter.x;
    //particle_data_buffer[particle_index].parameter.y = emit_particle_butter[emit_index].parameter.y;
    //particle_data_buffer[particle_index].parameter.z = 1.0f;
    //particle_data_buffer[particle_index].parameter.w = 0.0f;

     //  ヘッダーの末端から取得
    uint header_index = 0;
    indirect_data_buffer.InterlockedAdd(IndirectArgumentsNumEmitParticleIndex, 1, header_index);

    //  パーティクル生成処理
    particle_header_buffer[header_index].alive = 1; //  生存フラグ
    particle_header_buffer[header_index].particle_index = particle_index; //  パーティクルデータバッファの座標
    particle_header_buffer[header_index].depth = 1; //  深度
    particle_header_buffer[header_index].dummy = 0; //  空き

    //  パーティクル生成処理
    particle_data_buffer[particle_index].parameter.x = emit_particle_butter[emit_index].parameter.x;
    particle_data_buffer[particle_index].parameter.y = emit_particle_butter[emit_index].parameter.y;
    particle_data_buffer[particle_index].parameter.z = emit_particle_butter[emit_index].parameter.z;
    particle_data_buffer[particle_index].parameter.w = emit_particle_butter[emit_index].parameter.w;
    
    particle_data_buffer[particle_index].position = emit_particle_butter[emit_index].position;
    particle_data_buffer[particle_index].rotation = emit_particle_butter[emit_index].rotation;
    particle_data_buffer[particle_index].scale = emit_particle_butter[emit_index].scale;
    particle_data_buffer[particle_index].posVelocity = emit_particle_butter[emit_index].posVelocity;
    particle_data_buffer[particle_index].posAcceleration = emit_particle_butter[emit_index].posAcceleration;

    particle_data_buffer[particle_index].rotVelocity = emit_particle_butter[emit_index].rotVelocity;
    particle_data_buffer[particle_index].rotAcceleration = emit_particle_butter[emit_index].rotAcceleration;

    particle_data_buffer[particle_index].scaleVelocity = emit_particle_butter[emit_index].scaleVelocity;
    particle_data_buffer[particle_index].scaleAcceleration = emit_particle_butter[emit_index].scaleAcceleration;
    particle_data_buffer[particle_index].color = emit_particle_butter[emit_index].color;
    
    uint type = (uint) (emit_particle_butter[emit_index].parameter.x + 0.5f);
    float w = 1.0f / texture_split_count.x;
    float h = 1.0f / texture_split_count.y;
    float2 uv = float2((type % texture_split_count.x) * w, (type / texture_split_count.x) * h);
    
    particle_data_buffer[particle_index].texcoord.xy = uv;
    particle_data_buffer[particle_index].texcoord.zw = float2(w, h);
}
