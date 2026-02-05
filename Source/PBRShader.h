#pragma once
#include "Shader.h"

class PBRShader : public Shader
{
public:
    PBRShader(ID3D11Device* device);
    ~PBRShader() override = default;

    // 開始処理
    void Begin(const RenderContext& rc) override;

    // 更新処理
    void Update(const RenderContext& rc, const Model::Mesh& mesh) override;

    // 終了処理
    void End(const RenderContext& rc) override;

private:
    struct CbMesh
    {
        DirectX::XMFLOAT4 materialColor;
        DirectX::XMFLOAT3 emissiveColor;
        DirectX::XMFLOAT4 ambientColor;
        float metalness;
        float roughness;
        float adjustMetalness; // 福井先生にいらないって言われたけど消すとめんどいことなるから残してる
        float adjustRoughness;
        float oclusionStrength;
        float alphaCutoff;

        int hasBaseColorTexture;
        int hasEmissiveTexture;
        int hasMetalnessRoughnessTexture;
        int hasOcclusionTexture;
        int hasNormalTexture;

        float pad[2];
    };
    CbMesh cbMesh;

    // シャドウマップ
    struct shadowmap_constants
    {
        DirectX::XMFLOAT4X4    light_view_projection;
        DirectX::XMFLOAT4     shadow_color;
        float                shadow_attenuation;
        float                shadow_bias;
        DirectX::XMFLOAT2    shadow_dummy;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> shadowmap_constant_buffer;

    Microsoft::WRL::ComPtr<ID3D11VertexShader>        vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>        pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>        inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>            meshConstantBuffer;

    // IBL用のテクスチャ
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuse_iem_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specular_pmrem_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> lut_ggx_shader_resource_view;

    // シャドウマップ関係
    Microsoft::WRL::ComPtr<ID3D11Buffer> shadowmap_scene_constant_buffer;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowmap_depth_stencil_view;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowmap_shader_resource_view;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowmap_sampler_state;
};