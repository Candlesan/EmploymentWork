#include "ShadowMap.h"
#include "System/Core/Misc.h"

ShadowMap::ShadowMap(ID3D11Device* device, UINT width, UINT height)
{
    HRESULT hr = S_OK;

    this->width = width;
    this->height = height;

    // -------------------------------------------------------
    // 深度バッファ用テクスチャ作成
    // DSV（書き込み）とSRV（読み取り）の両方に使うためTYPELESS
    // -------------------------------------------------------
    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    hr = device->CreateTexture2D(&desc, nullptr, depthBuffer.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    // -------------------------------------------------------
    // 深度ステンシルビュー（DSV）作成
    // シャドウマップ描画時に深度値を書き込む窓口
    // -------------------------------------------------------
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    hr = device->CreateDepthStencilView(depthBuffer.Get(), &dsvDesc, depthStencilView.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    // -------------------------------------------------------
    // シェーダーリソースビュー（SRV）作成
    // 通常描画時にシャドウマップをPSから読み取る窓口
    // -------------------------------------------------------
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;

    hr = device->CreateShaderResourceView(depthBuffer.Get(), &srvDesc, shaderResourceView.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    // -------------------------------------------------------
    // サンプラーステート作成
    // BorderColorをFLT_MAXにしてシャドウマップ範囲外を「影なし」扱いにする
    // -------------------------------------------------------
    D3D11_SAMPLER_DESC samplerDesc{};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy = 16;
    samplerDesc.BorderColor[0] = FLT_MAX;
    samplerDesc.BorderColor[1] = FLT_MAX;
    samplerDesc.BorderColor[2] = FLT_MAX;
    samplerDesc.BorderColor[3] = FLT_MAX;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    hr = device->CreateSamplerState(&samplerDesc, samplerState.GetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

    // -------------------------------------------------------
    // ビューポート設定（シャドウマップ解像度に合わせる）
    // -------------------------------------------------------
    viewport.Width = static_cast<float>(width);
    viewport.Height = static_cast<float>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
}

// シャドウマップ描画モードに切り替える
void ShadowMap::Active(ID3D11DeviceContext* dc)
{
    // slot 5のSRVを先にnullにしてDSVとの競合を防ぐ
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    dc->PSSetShaderResources(5, 1, nullSRV);

    // 深度バッファをクリアしてシャドウマップ用のDSVをOMにセット
    // RTVはnull（深度値だけ書ければいいので色の出力先は不要）
    dc->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
    dc->RSSetViewports(1, &viewport);
    dc->OMSetRenderTargets(0, nullptr, depthStencilView.Get());
}

// 通常描画モードに戻す
void ShadowMap::Deactive(ID3D11DeviceContext* dc)
{
    // シャドウマップのDSVをOMから切り離す
    // （切り離さないとSRVとして読もうとした時に競合が起きる）
    dc->OMSetRenderTargets(0, nullptr, nullptr);

    // slot 5のSRVも念のためクリア
    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    dc->PSSetShaderResources(5, 1, nullSRV);
}