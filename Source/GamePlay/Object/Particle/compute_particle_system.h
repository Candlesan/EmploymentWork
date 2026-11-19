#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <directxmath.h>

#include <vector>
#include <algorithm>

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
        DirectX::XMFLOAT4 rotation = { 0, 0, 0, 0 };// 回転情報
        DirectX::XMFLOAT4 scale = { 1, 1, 1, 0 }; // 拡縮情報 

        DirectX::XMFLOAT4 posVelocity = { 0, 0, 0, 0 };     // 位置の初速
        DirectX::XMFLOAT4 posAcceleration = { 0, 0, 0, 0 }; // 位置の加速度

        DirectX::XMFLOAT4 rotVelocity = { 0, 0, 0, 0 };     // 回転初速
        DirectX::XMFLOAT4 rotAcceleration = { 0, 0, 0, 0 }; // 回転加速度

        DirectX::XMFLOAT4 scaleVelocity = { 0, 0, 0, 0 };       // 拡大の初速
        DirectX::XMFLOAT4 scaleAcceleration = { 0, 0, 0, 0 };   // 拡大の加速度

        // 単一イージング
        DirectX::XMFLOAT4 renderScale = { 1.0f, 1.0f, 1.0f, 1.0f };				// スケール(描画)
        float scaleUniformStart = 1.0f;	// 始点：中心
        float scaleUniformStartRandom = 0.0f; // 始点：振れ幅

        float scaleUniformEnd = 1.0f; // 終点：中心
        float scaleUniformEndRandom = 0.0f;	// 終点：振れ幅
        DirectX::XMFLOAT4 scale_param = { -1, 0, 0, 0 }; // x:イージングの速度, y: 寿命, z:空き, w:空き


        DirectX::XMFLOAT4 color = { 1, 1, 1, 1 }; // 色
        DirectX::XMFLOAT4 start_color = { 1, 1, 1, 1 }; // 始点の色
        DirectX::XMFLOAT4 end_color = { 1, 1, 1, 1 }; // 終点の色
        DirectX::XMFLOAT4 color_param = { -1, 0, 0, 0 }; // x:色イージングの始点・終点の速度, y:寿命, z:空き, w:空き

        // x:フェードイン秒数, y:フェードアウト秒数, z:フェードインの速度設定（始点・終点速度）, w:フェードアウトの速度設定（始点・終点速度）
        DirectX::XMFLOAT4 fade_param;

	};

    // パーティクル構造体
    // アプリケーション側では使用しないが、形式として必要なためここで宣言しておく
    struct particle_data
    {
        DirectX::XMFLOAT4 parameter; // x : パーティクル処理タイプ,y：生存時間,zw：空き

        DirectX::XMFLOAT4 position; // 生成座標
        DirectX::XMFLOAT4 rotation; // 拡縮情報
        DirectX::XMFLOAT4 scale; // 回転情報

        DirectX::XMFLOAT4 posVelocity = { 0, 0, 0, 0 };     // 位置の初速
        DirectX::XMFLOAT4 posAcceleration = { 0, 0, 0, 0 }; // 位置の加速度

        DirectX::XMFLOAT4 rotVelocity = { 0, 0, 0, 0 };     // 回転初速
        DirectX::XMFLOAT4 rotAcceleration = { 0, 0, 0, 0 }; // 回転加速度

        DirectX::XMFLOAT4 scaleVelocity = { 0, 0, 0, 0 };       // 拡大の初速
        DirectX::XMFLOAT4 scaleAcceleration = { 0, 0, 0, 0 };   // 拡大の加速度

        // 単一イージング
        DirectX::XMFLOAT4 renderScale = { 1.0f, 1.0f, 1.0f, 1.0f };				// スケール(描画)
        float scaleUniformStart = 1.0f;										// 始点：中心
        float scaleUniformStartRandom = 0.0f;								// 始点：振れ幅

        float scaleUniformEnd = 1.0f;										// 終点：中心
        float scaleUniformEndRandom = 0.0f;									// 終点：振れ幅
        DirectX::XMFLOAT4 scale_param; // x:イージングの速度, y: 空き, z:空き, w:空き

        DirectX::XMFLOAT4 texcoord; // UV座標

        DirectX::XMFLOAT4 color = { 1, 1, 1, 1 }; // 色
        DirectX::XMFLOAT4 renderColor = { 1, 1, 1, 1 }; // 色
        DirectX::XMFLOAT4 start_color = { 1, 1, 1, 1 }; // 始点の色
        DirectX::XMFLOAT4 end_color = { 1, 1, 1, 1 }; // 終点の色
        DirectX::XMFLOAT4 color_param = { -1, 0, 0, 0 }; // x:色イージングの始点速度, y:色イージングの終点速度, z:空き, w:空き

        // x:フェードイン秒数, y:フェードアウト秒数, z:フェードインの速度設定（始点・終点速度）, w:フェードアウトの速度設定（始点・終点速度）
        DirectX::XMFLOAT4 fade_param;
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
        float elapsed_time;                     // 経過時間
        DirectX::XMUINT2 texture_split_count;   // テクスチャの分割数
        UINT system_num_particles;              // パーティクル総数
        UINT total_emit_count;		            //	現在のフレームでのパーティクル総生成数

        DirectX::XMFLOAT3 parent_delta;      // 親の位置

        DirectX::XMFLOAT4X4 parent_world; // 親のワールド行列
        UINT local_space; // 1 = 相対位置で動かして、描くときに parent_world 

        UINT dissolve_mode;
        float dissolve_softness;

        UINT  trail_head;        // 今書いているスロット
        UINT  trail_points;      // １本あたりの点の数
        float trail_width;       // 帯の幅
        UINT  trail_enable;      // １＝ 履歴を書く
        UINT  trail_padding;

        DirectX::XMFLOAT4 force_vortex;   // xyz:軸, w:強さ
        DirectX::XMFLOAT4 force_attract;  // xyz:中心(ローカル), w:強さ(負で反発)
        DirectX::XMFLOAT4 force_noise;    // x:強さ, y:周波数, z:時間, w:空き
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
    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> sort_b2_shader;
    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> sort_c2_shader;

    //	DrawIndirectを用いるため、RWStrcturedBufferを用いる物に変更
    Microsoft::WRL::ComPtr<ID3D11Buffer> indirect_data_buffer;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> indirect_data_unordered_access_view;

    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> begin_frame_shader;
    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> end_frame_shader;

public:
    compute_particle_system(ID3D11Device* device, UINT particle_count,
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>shader_resource_view, DirectX::XMUINT2 splot_count =
        DirectX::XMUINT2(1, 1));
    ~compute_particle_system();

    void emit(const emit_particle_data& data);
    void update(ID3D11DeviceContext* immdiate_context, DirectX::XMFLOAT3 parent_delta, float elapsedtime);
    void render(ID3D11DeviceContext* immdiate_context);
    void render_trail(ID3D11DeviceContext* immediate_context);

    void clear(ID3D11DeviceContext* immediate_context);

    // ゲッターセッター
    void set_texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv, DirectX::XMUINT2 split_count = { 1, 1 })
    {
        shader_resource_view = srv;
        texture_split_count = split_count;
    }

    void set_parent(const DirectX::XMFLOAT4X4& world, bool local)
    {
        parent_world = world;
        local_space = local;
    }

    void set_dissolve(int mode, float value)
    {
        dissolve_mode = mode;
        dissolve_softness = value;
    }

    void set_trail(bool enable, float width, float interval)
    {
        trail_enable = enable;
        trail_width = width;
        trail_interval = (std::max)(interval, 0.0001f); // 0だとupdate()のwhileが止まらない
    }

    void set_forces(const DirectX::XMFLOAT4& vortex,
        const DirectX::XMFLOAT4& attract,
        const DirectX::XMFLOAT4& noise)
    {
        force_vortex = vortex;
        force_attract = attract;
        force_noise = noise;
    }

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
    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> init_shader;
    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> emit_shader;
    static Microsoft::WRL::ComPtr<ID3D11ComputeShader> update_shader;

    static Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
    static Microsoft::WRL::ComPtr<ID3D11GeometryShader> geometry_shader;
    static Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;

    // 親子関係
    DirectX::XMFLOAT4X4 parent_world = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    bool local_space = false;

    // ディゾルブ
    int dissolve_mode = 0;
    float dissolve_softness = 0.05f;

    // トレイル
    static constexpr UINT TrailPoints = 24;
    Microsoft::WRL::ComPtr<ID3D11Buffer> trail_history_buffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> trail_history_srv;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> trail_history_uav;
    
    UINT  trail_head = 0;        // 今書いているスロット
    float trail_timer = 0.0f;
    float trail_interval = 0.02f; // 何秒ごとに点を固定するか
    float trail_width = 0.3f;
    bool  trail_enable = false;

    DirectX::XMFLOAT4 force_vortex = {0, 1, 0, 0};   // xyz:軸, w:強さ
    DirectX::XMFLOAT4 force_attract = {0, 0, 0, 0};  // xyz:中心(ローカル), w:強さ(負で反発)
    DirectX::XMFLOAT4 force_noise = {0, 1, 0, 0};    // x:強さ, y:周波数, z:時間, w:空き
    float noise_time = 0.0f;

    static Microsoft::WRL::ComPtr<ID3D11VertexShader> trail_vertex_shader;
    static Microsoft::WRL::ComPtr<ID3D11PixelShader>  trail_pixel_shader;
};
