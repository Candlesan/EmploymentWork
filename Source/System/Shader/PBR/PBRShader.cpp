#include "System/Shader/PBR/PBRShader.h"
#include "System/Graphic/GpuResourceUtils.h"
#include "System/Graphic/Graphics.h"

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

    // ピクセルシェーダー
    GpuResourceUtils::LoadPixelShader(
        device,
        "Data/Shader/PBRShader_ps.cso",
        pixelShader.GetAddressOf());

    // メッシュ用定数バッファ
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CbMesh),
        meshConstantBuffer.GetAddressOf());

    // シャドウマップ用定数バッファ
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(shadowmap_constants),
        shadowmap_constant_buffer.GetAddressOf());

    // IBLテクスチャ読み込み（拡散反射・鏡面反射・LUT）
    {
        D3D11_TEXTURE2D_DESC texture2d_desc;
        //GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/BlueSky/diffuse_iem.dds", diffuse_iem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        //GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/BlueSky/specular_pmrem.dds", specular_pmrem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        //GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/BlueSky/lut_ggx.dds", lut_ggx_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/Night/diffuse_iem.dds", diffuse_iem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/Night/specular_pmrem.dds", specular_pmrem_shader_resource_view.GetAddressOf(), &texture2d_desc);
        GpuResourceUtils::LoadTexture(device, "Data/Sprite/IBLTexture/Night/lut_ggx.dds", lut_ggx_shader_resource_view.GetAddressOf(), &texture2d_desc);
    }
}

// 開始処理
void PBRShader::Begin(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // シェーダー・入力レイアウト設定
    dc->IASetInputLayout(inputLayout.Get());
    dc->VSSetShader(vertexShader.Get(), nullptr, 0);

    if (rc.isShadowPass)
    {
        dc->PSSetShader(nullptr, nullptr, 0);
    }
    else
    {
        dc->PSSetShader(pixelShader.Get(), nullptr, 0);
    }

    // メッシュ用定数バッファをb13にセット
    ID3D11Buffer* cbs[] = { meshConstantBuffer.Get() };
    dc->PSSetConstantBuffers(13, _countof(cbs), cbs);

    // -------------------------------------------------------
    // シャドウマップ関係
    // VSにはlight_view_projection、PSにはshadow_biasなどが必要
    // -------------------------------------------------------
    {
        ID3D11Buffer* shadowCB = shadowmap_constant_buffer.Get();
        dc->VSSetConstantBuffers(5, 1, &shadowCB);
        dc->PSSetConstantBuffers(5, 1, &shadowCB);

        // シャドウマップ用サンプラーをslot 5にセット
        ID3D11SamplerState* sampler = Graphics::Instance().GetStageShadowMap()->GetSamplerState();
        dc->PSSetSamplers(5, 1, &sampler);

        // shadowMapDataがある時だけSRVをslot 5にセット
        // （シャドウマップ描画中はnullのままにしてDSVとの競合を防ぐ）
        if (rc.stageShadowMapData != nullptr)
        {
            ID3D11ShaderResourceView* srv = rc.stageShadowMapData->shadowMap;
            dc->PSSetShaderResources(5, 1, &srv);
        }
        if (rc.characterShadowMapData != nullptr)
        {
            ID3D11ShaderResourceView* srv = rc.characterShadowMapData->shadowMap;
            dc->PSSetShaderResources(6, 1, &srv);
        }
    }
}

// 更新処理
void PBRShader::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // -------------------------------------------------------
    // メッシュ用定数バッファ更新（マテリアル情報を詰める）
    // -------------------------------------------------------
    CbMesh cbMesh{};
    cbMesh.materialColor = mesh.material->baseColor;
    cbMesh.emissiveColor = mesh.material->emissiveColor;
    cbMesh.ambientColor = { 1, 1, 1, rc.ambientIntensity };
    cbMesh.metalness = mesh.material->metalness;
    cbMesh.roughness = mesh.material->roughness;
    cbMesh.adjustMetalness = rc.pbrMetalness + mesh.material->adjustMetalness;
    cbMesh.adjustRoughness = rc.pbrRoughness + mesh.material->adjustRoughness;
    cbMesh.oclusionStrength = mesh.material->occlusionStrength;
    cbMesh.alphaCutoff = mesh.material->alphaCutoff;

    // テクスチャの有無をシェーダーに伝えるフラグ
    cbMesh.hasBaseColorTexture = mesh.material->baseMap ? 1 : 0;
    cbMesh.hasEmissiveTexture = mesh.material->emissiveMap ? 1 : 0;
    cbMesh.hasMetalnessRoughnessTexture = mesh.material->metalnessRoughnessMap ? 1 : 0;
    cbMesh.hasOcclusionTexture = mesh.material->occlusionMap ? 1 : 0;
    cbMesh.hasNormalTexture = mesh.material->normalMap ? 1 : 0;

    dc->UpdateSubresource(meshConstantBuffer.Get(), 0, 0, &cbMesh, 0, 0);

    // -------------------------------------------------------
    // シャドウマップ用定数バッファ更新
    // -------------------------------------------------------
    if (!rc.isShadowPass)
    {
        shadowmap_constants cbShadow{};
        cbShadow.stage_shadow_attenuation = 1.0f;   // 影データがない＝影の影響なし
        cbShadow.character_shadow_attenuation = 1.0f;

        if (rc.stageShadowMapData)
        {
            cbShadow.stage_light_view_projection = rc.stageShadowMapData->lightViewProjection;
            cbShadow.stage_shadow_bias = rc.stageShadowMapData->shadowBias;
            cbShadow.stage_shadow_attenuation = rc.stageShadowMapData->shadowAttenuation;
            cbShadow.shadow_color = rc.stageShadowMapData->shadowColor;
        }
        if (rc.characterShadowMapData)
        {
            cbShadow.character_light_view_projection = rc.characterShadowMapData->lightViewProjection;
            cbShadow.character_shadow_bias = rc.characterShadowMapData->shadowBias;
            cbShadow.character_shadow_attenuation = rc.characterShadowMapData->shadowAttenuation;
        }
        dc->UpdateSubresource(shadowmap_constant_buffer.Get(), 0, 0, &cbShadow, 0, 0);
    }

    // マテリアルテクスチャをslot 0〜4にセット
    ID3D11ShaderResourceView* srvs[] =
    {
        mesh.material->baseMap.Get(),              
        mesh.material->emissiveMap.Get(),          
        mesh.material->metalnessRoughnessMap.Get(),
        mesh.material->occlusionMap.Get(),         
        mesh.material->normalMap.Get(),            
    };
    dc->PSSetShaderResources(0, _countof(srvs), srvs);

    // IBLテクスチャをslot 33〜35にセット
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

    // シェーダー・入力レイアウト解除
    dc->VSSetShader(nullptr, nullptr, 0);
    dc->PSSetShader(nullptr, nullptr, 0);
    dc->IASetInputLayout(nullptr);

    // 定数バッファ解除（b13）
    ID3D11Buffer* nullCBs[] = { nullptr };
    dc->PSSetConstantBuffers(13, _countof(nullCBs), nullCBs);

    // マテリアルSRV解除（slot 0〜4）
    ID3D11ShaderResourceView* nullSRVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    dc->PSSetShaderResources(0, _countof(nullSRVs), nullSRVs);

    // シャドウマップSRV解除（slot 5, 6）
    ID3D11ShaderResourceView* srvs_null[] = { nullptr, nullptr };
    dc->PSSetShaderResources(5, 2, srvs_null);

    // IBL SRV解除（slot 33〜35）
    ID3D11ShaderResourceView* nullIBLSRVs[3] = { nullptr, nullptr, nullptr };
    dc->PSSetShaderResources(33, _countof(nullIBLSRVs), nullIBLSRVs);

    // シャドウマップサンプラー解除（slot 5）
    ID3D11SamplerState* nullSampler[] = { nullptr };
    dc->PSSetSamplers(5, 1, nullSampler);
}