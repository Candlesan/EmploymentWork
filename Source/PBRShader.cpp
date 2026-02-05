#include "PBRShader.h"
#include "GpuResourceUtils.h"
#include "Graphics.h"
#include "Sprite.h"
#include "ShadowMap.h"


// コンストラクタ
PBRShader::PBRShader(ID3D11Device* device)
{
    // 頂点シェーダー
    GpuResourceUtils::LoadVertexShader(
        device,
        "Data/Shader/PBRShader_vs.cso",
        Model::InputElementDescs.data(),
        static_cast<UINT>(Model::InputElementDescs.size()),
        inputLayout.GetAddressOf(),
        vertexShader.GetAddressOf());

    GpuResourceUtils::LoadPixelShader(
        device,
        "Data/Shader/PBRShader_ps.cso",
        pixelShader.GetAddressOf());

    // メッシュ用定数バッファ
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CbMesh),
        meshConstantBuffer.GetAddressOf());

    // シャドウマップ
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(shadowmap_constants),
        shadowmap_constant_buffer.GetAddressOf()
    );

    // IBLテクスチャを読み込み
    {
        D3D11_TEXTURE2D_DESC texture2d_desc;
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBL/morning/diffuse_iem.dds", diffuse_iem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBL/morning/specular_pmrem.dds", specular_pmrem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBL/morning/lut_ggx.dds", lut_ggx_shader_resource_view.GetAddressOf(), &texture2d_desc);
    }
}

// 開始処理
void PBRShader::Begin(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // シェーダー設定
    dc->IASetInputLayout(inputLayout.Get());
    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);

    // 定数バッファ設定
    ID3D11Buffer* cbs[] =
    {
        meshConstantBuffer.Get(),
    };
    dc->PSSetConstantBuffers(13, _countof(cbs), cbs);

    // シャドウマップ関係
    {
        ID3D11Buffer* shadowCB = shadowmap_constant_buffer.Get();
        dc->VSSetConstantBuffers(5, 1, &shadowCB); // VSにも必要（light_view_projectionのため）
        dc->PSSetConstantBuffers(5, 1, &shadowCB); // PSにも必要（shadow_biasなどのため

        // サンプラーステート設定 
        ID3D11SamplerState* sampler = Graphics::Instance().GetShadowMap()->GetSamplerState();
        dc->PSSetSamplers(5, 1, &sampler);

        // シェーダーリソースビュー設定 
        {
            ID3D11ShaderResourceView* srv = Graphics::Instance().GetShadowMap()->GetShaderResourceView();
            dc->PSSetShaderResources(5, 1, &srv);
        }
    }
}

// 更新処理
void PBRShader::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // メッシュ用定数バッファ更新
    CbMesh cbMesh{};
    cbMesh.materialColor = mesh.material->baseColor;
    cbMesh.emissiveColor = mesh.material->emissiveColor;
    cbMesh.ambientColor = { 1, 1, 1, 1 };
    cbMesh.metalness = mesh.material->metalness;
    cbMesh.roughness = mesh.material->roughness;
    cbMesh.adjustMetalness = rc.pbrMetalness;
    cbMesh.adjustRoughness = rc.pbrRoughness;
    cbMesh.oclusionStrength = mesh.material->occlusionStrength;
    cbMesh.alphaCutoff = mesh.material->alphaCutoff;

    cbMesh.hasBaseColorTexture = mesh.material->baseMap ? 1 : 0;
    cbMesh.hasEmissiveTexture = mesh.material->emissiveMap ? 1 : 0;
    cbMesh.hasMetalnessRoughnessTexture = mesh.material->metalnessRoughnessMap ? 1 : 0;
    cbMesh.hasOcclusionTexture = mesh.material->occlusionMap ? 1 : 0;
    cbMesh.hasNormalTexture = mesh.material->normalMap ? 1 : 0;

    dc->UpdateSubresource(meshConstantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

    // シャドウマップ用定数バッファ更新
    if (rc.shadowMapData != nullptr)
    {
        shadowmap_constants cbShadow{};
        cbShadow.light_view_projection = rc.shadowMapData->lightViewProjection;
        cbShadow.shadow_bias = rc.shadowMapData->shadowBias;
        cbShadow.shadow_attenuation = rc.shadowMapData->shadowAttenuation;
        cbShadow.shadow_color = rc.shadowMapData->shadowColor;

        dc->UpdateSubresource(shadowmap_constant_buffer.Get(), 0, 0, &cbShadow, 0, 0);

        // Beginで設定済みだが、最新の情報で上書き (特にサンプラーが変わる場合に有効)
        dc->PSSetSamplers(5, 1, &rc.shadowMapData->shadowSampler);
        dc->PSSetShaderResources(5, 1, &rc.shadowMapData->shadowMap);
    }

    // シェーダーリソースビュー
    ID3D11ShaderResourceView* srvs[] =
    {
        mesh.material->baseMap.Get(),
        mesh.material->emissiveMap.Get(),
        mesh.material->metalnessRoughnessMap.Get(),
        mesh.material->occlusionMap.Get(),
        mesh.material->normalMap.Get(),
    };
    dc->PSSetShaderResources(0, _countof(srvs), srvs);

    // IBLテクスチャをシェーダーに送る
    ID3D11ShaderResourceView* shader_resource_views[] =
    {
        diffuse_iem_shader_resource_view.Get(),
        specular_pmrem_shader_resource_view.Get(),
        lut_ggx_shader_resource_view.Get(),
    };
    dc->PSSetShaderResources(33, _countof(shader_resource_views), shader_resource_views);
}

// 終了処理
void PBRShader::End(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // シェーダー設定解除
    dc->VSSetShader(nullptr, nullptr, 0);
    dc->PSSetShader(nullptr, nullptr, 0);
    dc->IASetInputLayout(nullptr);

    // 定数バッファ設定解除
    ID3D11Buffer* cbs[] = { nullptr };
    dc->PSSetConstantBuffers(13, _countof(cbs), cbs);

    // シェーダーリソースビュー設定解除
    ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    dc->PSSetShaderResources(0, 5, nullSRVs);

    ID3D11ShaderResourceView* srvs_null[] = { nullptr };
    // ★ t5 の解除を追加 ★
    dc->PSSetShaderResources(5, 1, srvs_null);
}
