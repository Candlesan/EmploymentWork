#include "compute_particle.hlsli"

StructuredBuffer<particle_data> particle_data_buffer : register(t0);
StructuredBuffer<particle_header> particle_header_buffer : register(t1);
StructuredBuffer<float4> trail_history : register(t2);

// i個前の位置（ワールド座標）
float3 LoadTrailPoint(uint id, uint i)
{
    // リングバッファから過去の座標を取り出す
    // 現在最新の書き込み位置からi個分さかのぼったインデックスを「％」で計算して
    // 座標を取り出している
    uint slot = (trail_head + trail_points - i) % trail_points;
    float3 p = trail_history[id * trail_points + slot].xyz;
    
    // local_space が有効の時parent_worldを掛けてワールド座標に変換してから返す
    if (local_space != 0)
    {
        p = mul(float4(p, 1), parent_world).xyz; // 相対位置モードはワールドに戻す
    }
    
    return p;
}

PS_IN main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
    PS_IN vout = (PS_IN) 0;
    
    // instance_idはヘッダーの番号。死んでいる粒は画面外に飛ばして描かない
    particle_header header = particle_header_buffer[instance_id];
    if (header.alive == 0)
    {
        vout.position = float4(0, 0, -1, -1); // z < 0 なのでクリップされる
        return vout;
    }
    
    uint id = header.particle_index;
    particle_data p = particle_data_buffer[id];
    
    // 帯のどこに位置する頂点かを決める
    uint i = vertex_id / 2; // 何個前の点か（０が最新）頂点番号を２で割ることで２頂点ずつ履歴を参照できる
    float side = (vertex_id & 1) ? 1.0f : -1.0f; // 偶数が左、奇数が右
    float t = i / (float) (trail_points - 1); // 0:頭 → 1:しっぽ
    
    float3 pos = LoadTrailPoint(id, i);
    float3 newer = LoadTrailPoint(id, (i == 0) ? 0 : i - 1);
    float3 older = LoadTrailPoint(id, min(i + 1, trail_points - 1));
    float3 tangent = newer - older; // 進む向き
    
    // 帯の横方向 = 進む向きとカメラへの向きの外積
    float3 cameraPos = inverseView._41_42_43;
    float3 right = cross(tangent, cameraPos - pos);
    float len = length(right);
    right = (len > 1e-6f) ? right / len : float3(0, 0, 0); // 長さ0の区間はつぶす
    
    // 幅：粒の大きさ×設定値×先細り
    float width = trail_width * p.renderScale.x * (1.0f - t);
    
    vout.position = mul(float4(pos + right * side * width * 0.5f, 1), viewProjection);
    vout.color = p.renderColor;
    vout.color.a *= (1.0f - t);
    vout.texcoord = float2(t, side * 0.5f + 0.5f); // x:長さ方向　y:幅方向(0～1)
    
    return vout;
}