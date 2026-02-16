#include "System/Renderer/Shader/PBR/PBRShader.h"
#include "System/Graphic/GpuResourceUtils.h"


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

    // IBLテクスチャを読み込み
    {
        D3D11_TEXTURE2D_DESC texture2d_desc;
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/diffuse_iem.dds", diffuse_iem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/specular_pmrem.dds", specular_pmrem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/lut_ggx.dds", lut_ggx_shader_resource_view.GetAddressOf(), &texture2d_desc);
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
    dc->PSSetShaderResources(0, _countof(nullSRVs), nullSRVs);

    // IBLのシェーダーリソースビュー設定解除
    ID3D11ShaderResourceView* nullIBLSRVs[3] = { nullptr, nullptr, nullptr};
    dc->PSSetShaderResources(33, _countof(nullIBLSRVs), nullSRVs);
}
