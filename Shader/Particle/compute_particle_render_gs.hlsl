#include "compute_particle.hlsli"

//  拡縮行列生成
float4x4 matrix_scaling(float3 scale)
{
    float4x4 m;
    m._11 = scale.x;
    m._12 = 0.0f;
    m._13 = 0.0f;
    m._14 = 0.0f;

    m._21 = 0.0f;
    m._22 = scale.y;
    m._23 = 0.0f;
    m._24 = 0.0f;

    m._31 = 0.0f;
    m._32 = 0.0f;
    m._33 = scale.z;
    m._34 = 0.0f;

    m._41 = 0.0f;
    m._42 = 0.0f;
    m._43 = 0.0f;
    m._44 = 1.0f;
    return m;
}

//  回転行列生成
float4x4 matrix_rotation_roll_pitch_yaw(float3 rotation)
{
    float cp, sp;
    float cy, sy;
    float cr, sr;
    sincos(rotation.x, sp, cp);
    sincos(rotation.y, sy, cy);
    sincos(rotation.z, sr, cr);

    float4x4 m;
    m._11 = cr * cy + sr * sp * sy;
    m._12 = sr * cp;
    m._13 = sr * sp * cy - cr * sy;
    m._14 = 0.0f;

    m._21 = cr * sp * sy - sr * cy;
    m._22 = cr * cp;
    m._23 = sr * sy + cr * sp * cy;
    m._24 = 0.0f;

    m._31 = cp * sy;
    m._32 = -sp;
    m._33 = cp * cy;
    m._34 = 0.0f;

    m._41 = 0.0f;
    m._42 = 0.0f;
    m._43 = 0.0f;
    m._44 = 1.0f;
    return m;
}

//  移動行列生成
float4x4 matrix_translation(float3 translation)
{
    float4x4 m;
    m._11 = 1.0f;
    m._12 = 0.0f;
    m._13 = 0.0f;
    m._14 = 0.0f;

    m._21 = 0.0f;
    m._22 = 1.0f;
    m._23 = 0.0f;
    m._24 = 0.0f;

    m._31 = 0.0f;
    m._32 = 0.0f;
    m._33 = 1.0f;
    m._34 = 0.0f;

    m._41 = translation.x;
    m._42 = translation.y;
    m._43 = translation.z;
    m._44 = 1.0f;
    return m;
}

StructuredBuffer<particle_data> particle_data_buffer : register(t0); //  パーティクル管理バッファ
StructuredBuffer<particle_header> particle_header_buffer : register(t1);

[maxvertexcount(4)]
void main(point GS_IN gin[1], inout TriangleStream<PS_IN> output)
{
    uint vertex_id = gin[0].vertex_id;

    ////  生存していない場合はスケールを0にしておく
    //float3 scale = particle_data_buffer[vertex_id].parameter.z < 0.0f ? float3(0, 0, 0) : particle_data_buffer[vertex_id].scale.xyz;
    
    //  ヘッダーの particle_index がそのままパーティクルデータバッファのインデックスになる。
    bool is_alive = particle_header_buffer[vertex_id].alive != 0;
    vertex_id = particle_header_buffer[vertex_id].particle_index;

    //  生存していない場合はスケールを0にしておく
    float3 scale = !is_alive ? float3(0, 0, 0) : particle_data_buffer[vertex_id].scale.xyz;

    //  ビルボード行列生成(ビュー行列の逆行列で良い。ただし移動値はいらない)
    float4x4 billboard_matrix = inverseView;
    billboard_matrix._41_42_43 = float3(0, 0, 0);
    billboard_matrix._44 = 1.0f;

	//  ワールド行列生成
    float4x4 scale_matrix = matrix_scaling(scale);
    float4x4 rotation_matrix = mul(billboard_matrix, matrix_rotation_roll_pitch_yaw(particle_data_buffer[vertex_id].rotation.xyz));
    float4x4 translation_matrix = matrix_translation(particle_data_buffer[vertex_id].position.xyz);
    float4x4 world_matrix = mul(mul(scale_matrix, rotation_matrix), translation_matrix);
    float4x4 world_view_projection_matrix = mul(world_matrix, viewProjection);

    //  各種情報取得
    float4 texcoord = particle_data_buffer[vertex_id].texcoord;
    float4 color = particle_data_buffer[vertex_id].color;

    //  頂点生成
    static const float4 vertex_positions[4] =
    {
        float4(-0.5f, -0.5f, 0, 1),
        float4(+0.5f, -0.5f, 0, 1),
        float4(-0.5f, +0.5f, 0, 1),
        float4(+0.5f, +0.5f, 0, 1),
    };
    static const float2 vertex_texcoord[4] =
    {
        float2(0, 0),
        float2(1, 0),
        float2(0, 1),
        float2(1, 1),
    };
    for (uint i = 0; i < 4; i++)
    {
        PS_IN element;
        element.position = mul(vertex_positions[i], world_view_projection_matrix);
        element.texcoord = texcoord.xy + texcoord.zw * vertex_texcoord[i];
        element.color = color;
        output.Append(element);
    }
    output.RestartStrip();
}
