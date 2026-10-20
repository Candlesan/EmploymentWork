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
    
    bool is_alive = particle_header_buffer[vertex_id].alive != 0;
    if (!is_alive)
    {
        return; // 死んでいるものは4頂点の生成(Append)を行わない
    }
        
    //  ヘッダーの particle_index がそのままパーティクルデータバッファのインデックスになる。
    vertex_id = particle_header_buffer[vertex_id].particle_index;

    //  生存していない場合はスケールを0にしておく
    float3 scale = !is_alive ? float3(0, 0, 0) : particle_data_buffer[vertex_id].renderScale.xyz;
    
    float4x4 base_matrix;
    
    particle_data p = particle_data_buffer[vertex_id];
    float billboardType = p.rotation.w; // 0:なし 1:ビルボード 2:速度方向に伸ばす
    
    // 速度方向に伸ばす
    if (billboardType > 1.5f)
    {
        // ワールド座標の位置と速度（相対位置モードでは親の行列で直す）
        float3 worldPos = p.position.xyz;
        float3 velocity = p.posVelocity.xyz;
        if (local_space != 0)
        {
            worldPos = mul(float4(worldPos, 1), parent_world).xyz; // ワールド行列と親の行列を掛け算してxyzだけ取り出す
            velocity = mul(float4(velocity, 0), parent_world).xyz; // 速度やから向きだけを取り出す
        }
        
        float speed = length(velocity);
        float3 dir = (speed > 0.0001f) ? velocity / speed : float3(0, 1, 0); // 止まっていたら上向き
        
        // カメラへの向き(逆ビュー行列の移動成分がカメラの位置)
        float3 cameraPos = inverseView._41_42_43;
        float3 toCamera = normalize(cameraPos - worldPos); // カメラの位置
        
        // 幅の向き = 進む向きとカメラへの向きは直角
        float3 side = cross(dir, toCamera);
        float sideLen = length(side);
        side = (sideLen > 0.0001f) ? side / sideLen : float3(1, 0, 0); // 真正面から見ているときの保険
        
        float width = scale.x;
        float length_ = scale.y + speed * p.scale_param.z; // scale_param.zは伸ばす量
        
        // 粒の位置が先頭になるように、四角形の中心をうしろ
        float3 center = worldPos - dir * (length_ * 0.5f);
        
        static const float2 corners[4] =
        {
            float2(-0.5f, -0.5f),
            float2(+0.5f, -0.5f),
            float2(-0.5f, +0.5f),
            float2(+0.5f, +0.5f)
        };
        static const float2 uvs[4] =
        {
            float2(0, 0),
            float2(1, 0),
            float2(0, 1),
            float2(1, 1)
        };

        for (uint i = 0; i < 4; i++)
        {
            float3 v = center + side * (corners[i].x * width) + dir * (corners[i].y * length_);

            PS_IN element;
            element.position = mul(float4(v, 1), viewProjection);
            element.texcoord = p.texcoord.xy + p.texcoord.zw * uvs[i];
            element.color = p.renderColor;
            output.Append(element);
        }
        output.RestartStrip();
        return;
    }
    else if (particle_data_buffer[vertex_id].rotation.w > 0.5f)
    {
        // ビルボード行列生成(ビュー行列の逆行列で良い。ただし移動値はいらない)
        base_matrix = inverseView;
        base_matrix._41_42_43 = float3(0, 0, 0);
        base_matrix._44 = 1.0f;
    }
    else
    {
        // ビルボードをしないのであれば単位行列
        base_matrix = float4x4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    }

	//  ワールド行列生成
    float3 position = particle_data_buffer[vertex_id].position.xyz;
    float4x4 scale_matrix = matrix_scaling(scale);
    float4x4 rotation_matrix = mul(base_matrix, matrix_rotation_roll_pitch_yaw(particle_data_buffer[vertex_id].rotation.xyz));
    float4x4 translation_matrix = matrix_translation(particle_data_buffer[vertex_id].position.xyz);
    float4x4 world_matrix;
    
    if (local_space != 0 && particle_data_buffer[vertex_id].rotation.w > 0.5f)
    {
        // 相対位置　＋　ビルボード：位置だけ親行列を動かして、向きはカメラに向けたまま
        float3 worldPos = mul(float4(position, 1), parent_world).xyz;
        world_matrix = mul(mul(scale_matrix, rotation_matrix), matrix_translation(worldPos));
    }
    else if (local_space != 0)
    {
        // 相対位置　＋ビルボード無し：粒の行列ごと親の行列で動かす（親と一緒に回る）
        world_matrix = mul(mul(mul(scale_matrix, rotation_matrix), matrix_translation(position)), parent_world);
    }
    else
    {
        world_matrix = mul(mul(scale_matrix, rotation_matrix), translation_matrix);
    }
    
    float4x4 world_view_projection_matrix = mul(world_matrix, viewProjection);

    //  各種情報取得
    float4 texcoord = particle_data_buffer[vertex_id].texcoord;
    float4 color = particle_data_buffer[vertex_id].renderColor;

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
