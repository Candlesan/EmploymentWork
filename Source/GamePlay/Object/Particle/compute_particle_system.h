#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <directxmath.h>

#include <vector>

#include "System/Core/Misc.h"
#include "System/Graphic/GpuResourceUtils.h"

class compute_particle_system
{
public:
	// パーティクルスレッド数
	static constexpr UINT NumParticleThread = 1024;

	// パーティクル生成用構造体
	struct emit_particle_data
	{
        DirectX::XMFLOAT4 parameter; // x : パーティクル処理タイプ,y：生存時間,zw：空き

        DirectX::XMFLOAT4 position = { 0, 0, 0, 0 }; // 生成座標
        DirectX::XMFLOAT4 rotation = { 0, 0, 0, 0 }; // 拡縮情報
        DirectX::XMFLOAT4 scale = { 1, 1, 1, 0 }; // 回転情報


        DirectX::XMFLOAT4 velocity = { 0, 0, 0, 0 }; // 初速
        DirectX::XMFLOAT4 acceleration = { 0, 0, 0, 0 }; // 加速度

        DirectX::XMFLOAT4 color = { 1, 1, 1, 1 }; // 色情報

	};

    // パーティクル構造体
    // アプリケーション側では使用しないが、形式として必要なためここで宣言しておく
    struct particle_data
    {
        DirectX::XMFLOAT4 parameter; // x : パーティクル処理タイプ,y：生存時間,zw：空き

        DirectX::XMFLOAT4 position; // 生成座標
        DirectX::XMFLOAT4 rotation; // 拡縮情報
        DirectX::XMFLOAT4 scale; // 回転情報

        DirectX::XMFLOAT4 velocity; // 初速
        DirectX::XMFLOAT4 acceleration; // 加速度

        DirectX::XMFLOAT4 texcoord; // UV座標
        DirectX::XMFLOAT4 color; // 色情報
    };

    //	パーティクルヘッダー構造体
    struct particle_header
    {
        UINT	alive;			//	生存フラグ
        UINT	particle_index;	//	パーティクル番号
        float	depth;			//	深度
        UINT	dummy;
    };

    //	パーティクルヘッダーバッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> particle_header_buffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particle_header_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> particle_header_unordered_access_view;

    // 汎用情報定義
    struct common_constants
    {
        float elapsed_time; // 経過時間
        DirectX::XMUINT2 texture_split_count; // テクスチャの分割数
        UINT system_num_particles; // パーティクル総数
        UINT				total_emit_count;		//	現在のフレームでのパーティクル総生成数
        UINT				common_dummy[3];
    };

    //	DispatchIndirect用構造体
    using	dispatch_indirect = DirectX::XMUINT3;	//	UINT3で十分

    //	00バイト目：現在のパーティクル総数
    //	04バイト目：1F前のパーティクル総数
    //	08バイト目：パーティクル破棄数
    //	12バイト目：パーティクル生成用DispatchIndirect情報
    static	constexpr	UINT	NumCurrentParticleOffset = 0;
    static	constexpr	UINT	NumPreviousParticleOffset = NumCurrentParticleOffset + sizeof(UINT);
    static	constexpr	UINT	NumDeadParticleOffset = NumPreviousParticleOffset + sizeof(UINT);
    static	constexpr	UINT	EmitDispatchIndirectOffset = NumDeadParticleOffset + sizeof(UINT);
    //static	constexpr	UINT	DrawIndirectSize = EmitDispatchIndirectOffset + sizeof(dispatch_indirect);

    //	DrawInstanced用DrawIndirect用構造体
    struct draw_indirect
    {
        UINT	vertex_count_per_instance;
        UINT	instance_count;
        UINT	start_vertex_location;
        UINT	start_instance_location;
    };
    //	24バイト目：パーティクル更新用DispatchIndirect情報
    //	36バイト目：パーティクル生成時に使用するインデックス(Append/Consumeの代わり)
    //	40バイト目：DrawIndirect情報
    static	constexpr	UINT	UpdateDispatchIndirectOffset = EmitDispatchIndirectOffset + sizeof(dispatch_indirect);
    static	constexpr	UINT	NumEmitParticleIndexOffset = UpdateDispatchIndirectOffset + sizeof(dispatch_indirect);
    static	constexpr	UINT	DrawIndirectOffset = NumEmitParticleIndexOffset + sizeof(UINT);
    static	constexpr	UINT	DrawIndirectSize = DrawIndirectOffset + sizeof(draw_indirect);

    //	バイトニックソート情報定義
    struct bitonic_sort_constants
    {
        UINT	increment;
        UINT	direction;
        UINT	dummy[2];
    };
    static constexpr UINT BitonicSortB2Thread = 256;
    static constexpr UINT BitonicSortC2Thread = 512;

    Microsoft::WRL::ComPtr<ID3D11Buffer> bitonic_sort_constant_buffer;

    //Microsoft::WRL::ComPtr<ID3D11ComputeShader> sort_shader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> sort_b2_shader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> sort_c2_shader;

    //	DrawIndirectを用いるため、RWStrcturedBufferを用いる物に変更
    Microsoft::WRL::ComPtr<ID3D11Buffer> indirect_data_buffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> indirect_data_unordered_access_view;

    Microsoft::WRL::ComPtr<ID3D11ComputeShader> begin_frame_shader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> end_frame_shader;

public:
    compute_particle_system(ID3D11Device* device, UINT particle_count,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>shader_resource_view, DirectX::XMUINT2 splot_count =
        DirectX::XMUINT2(1, 1));
    ~compute_particle_system();

    void emit(const emit_particle_data& data);
    void update(ID3D11DeviceContext* immdiate_context, float elapsedtime);
    void render(ID3D11DeviceContext* immdiate_context);

private:
    UINT num_particles;
    UINT num_emit_particles;
    bool one_shot_initialize;
    DirectX::XMUINT2 texture_split_count;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;

    std::vector<emit_particle_data> emit_particles;
    Microsoft::WRL::ComPtr<ID3D11Buffer> common_constant_buffer;

    // パーティクルバッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> particle_data_buffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particle_data_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> particle_data_unordered_access_view;

    // 未使用パーティクル番号を格納氏らAppend/Cosumeバッファ
    Microsoft::WRL::ComPtr<ID3D11Buffer> particle_append_consume_buffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> particle_append_consume_unordered_access_view;

    // パーティクル生成情報を格納した
    Microsoft::WRL::ComPtr<ID3D11Buffer> particle_emit_buffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> particle_emit_shader_resource_view;

    // 各種シェーダー
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> init_shader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> emit_shader;
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> update_shader;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometry_shader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
};
