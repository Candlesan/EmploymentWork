#include "../Common/Scene.hlsli"

//  パーティクルスレッド数
static const int NumParticleThread = 1024;

//  生成パーティクル構造体
struct emit_particle_data
{
    float4 parameter; // x : パーティクル処理タイプ, y : 生存時間, zw : 空き

    float4 position;          // 生成座標
    float4 rotation;          // 拡縮情報
    float4 scale;             // 回転情報

    float4 posVelocity;       // 位置の初速
    float4 posAcceleration;   // 位置の加速度
    
    float4 rotVelocity;       // 回転の初速
    float4 rotAcceleration;   // 回転の加速度
    
    float4 scaleVelocity;     // 拡大の初速
    float4 scaleAcceleration; // 拡大の加速度
        
    float4 color; // 色
    float4 start_color; // 始点の色
    float4 end_color; // 終点の色
    float4 color_param; // x: 始点と終点の速度, y: 寿命 , z: 空き, w: 空き
    
    // フェード
    // x: フェードインの時間時間, y: フェードアウトの時間, z:寿命, w: フェードイン・フェードアウトの始点と終点の速度
    float4 fade_param; 
};

//  パーティクル構造体
struct particle_data
{
    float4 parameter; // x : パーティクル処理タイプ, y : 生存時間, z : UVスクロールX, w : UVスクロールY

    float4 position; // 生成座標
    float4 rotation; // 回転情報
    float4 scale; // 拡縮情報

    float4 posVelocity; // 位置の初速
    float4 posAcceleration; // 位置の加速度

    float4 rotVelocity; // 回転の初速
    float4 rotAcceleration; // 回転の加速度

    float4 scaleVelocity; // 拡大の初速
    float4 scaleAcceleration; // 拡大の加速度

    float4 texcoord; // UV座標
    
    float4 color; // 色
    float4 renderColor; // 色(描画用)
    float4 start_color; // 始点の色
    float4 end_color; // 終点の色
    float4 color_param; // x: 始点と終点の速度, y: 寿命, z: 空き, w: 空き
    
    float4 fade_param;
 };

//	パーティクルヘッダー構造体
struct particle_header
{
    uint alive; // 生存フラグ
    uint particle_index; // パーティクル番号
    float depth; // 深度
    uint dummy;
};

//  indirect_data_bufferへのアクセス用バイトオフセット
static const uint IndirectArgumentsNumCurrentParticle = 0;
static const uint IndirectArgumentsNumPreviousParticle = 4;
static const uint IndirectArgumentsNumDeadParticle = 8;
static const uint IndirectArgumentsEmitParticleDispatchIndirect = 12;

//	DrawInstanced用DrawIndirect用構造体
struct draw_indirect
{
    uint vertex_count_per_instance;
    uint instance_count;
    uint start_vertex_location;
    uint start_instance_location;
};
static const uint IndirectArgumentsUpdateParticleDispatchIndirect = 24;
static const uint IndirectArgumentsNumEmitParticleIndex = 36;
static const uint IndirectArgumentsDrawIndirect = 40;

//=========================================================================================
//  汎用情報
cbuffer COMPUTE_PARTICLE_COMMON_CONSTANT_BUFFER : register(b10)
{
    float elapsed_time;
    uint2 texture_split_count;
    uint system_num_particles;
    
    uint total_emit_count; // 生成予定のパーティクル数
    uint common_dummy[3];
};

//=========================================================================================
//  頂点シェーダーからジオメトリシェーダーに転送する情報
struct GS_IN
{
    uint vertex_id : VERTEX_ID;
};

//  ジオメトリシェーダーからピクセルシェーダーに転送する情報
struct PS_IN
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

//	バイトニックソート情報
cbuffer COMPUTE_PARTICLE_BITONIC_SORT_CONSTANT_BUFFER : register(b11)
{
    uint increment;
    uint direction;
    uint sort_dummy[2];
};
static const uint BitonicSortB2Thread = 256;
static const uint BitonicSortC2Thread = 512;
