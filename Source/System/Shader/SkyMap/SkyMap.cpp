#include "SkyMap.h"
#include "System/Graphic/GpuResourceUtils.h"
#include "System/Graphic/Graphics.h"


// コンストラクタ
SkyMap::SkyMap(ID3D11Device* device)
{
    // 入力レイアウト
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // 頂点シェーダー
    GpuResourceUtils::LoadVertexShader(
        device,
        "Data/Shader/SkyMap_VS.cso",
        inputElementDesc,
        _countof(inputElementDesc),
        inputLayout.GetAddressOf(),
        vertexShader.GetAddressOf());

    // ピクセルシェーダー
    GpuResourceUtils::LoadPixelShader(
        device,
        "Data/Shader/SkyMap_PS.cso",
        pixelShader.GetAddressOf());

    // スカイマップ用定数バッファ
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(SkymapConstns),
        skymap_constant_buffer.GetAddressOf());

    //　スカイマップ用の画像読み込み
    GpuResourceUtils::LoadTexture(
        device,
        "Data/Sprite/SkyMap/Sky.png",
        skymap_shader_resource_view.GetAddressOf(),
        &skymap_texture2d_desc);

    // スプライト初期化
    skymap_sprite = std::make_unique<Sprite>(device, skymap_shader_resource_view);
}

// 開始処理
void SkyMap::Begin(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // シェーダー設定
    dc->IASetInputLayout(inputLayout.Get());
    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);

    // サンプラー設定
    ID3D11SamplerState* samplers[] = { rc.renderState->GetSamplerState(SamplerState::LinearWrap) };
    dc->PSSetSamplers(0, 1, samplers);

    // 定数バッファ設定
    ID3D11Buffer* cbs[] = { skymap_constant_buffer.Get() };
    dc->VSSetConstantBuffers(8, 1, cbs); 
    dc->PSSetConstantBuffers(8, 1, cbs);
}

// 更新処理
void SkyMap::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // カメラから見た視点の逆行列を計算
    DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
    DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
    V.r[3] = DirectX::XMVectorSet(0, 0, 0, 1);

    // 定数バッファ更新
    SkymapConstns skymap{};
    DirectX::XMStoreFloat4x4(&skymap.inverse_view_projection, DirectX::XMMatrixInverse(nullptr, V * P));
    skymap.cameraPosition = DirectX::XMFLOAT4(
        rc.camera->GetEye().x,
        rc.camera->GetEye().y,
        rc.camera->GetEye().z,
        1.0f);
    dc->UpdateSubresource(skymap_constant_buffer.Get(), 0, 0, &skymap, 0, 0);

    // シェーダーリソースビュー設定
    ID3D11ShaderResourceView* srvs[] = { skymap_shader_resource_view.Get() };
    dc->PSSetShaderResources(0, 1, srvs);
}

// 描画処理
void SkyMap::Draw(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // 頂点更新
    skymap_sprite->Update(
        0, 0, 1.0f,
        Graphics::Instance().GetScreenWidth(),
        Graphics::Instance().GetScreenHeight(),
        0, 0,
        Graphics::Instance().GetScreenWidth(),
        Graphics::Instance().GetScreenHeight(),
        0,
        1, 1, 1, 1);

    Begin(rc);

    Model::Mesh dummyMesh{};
    Update(rc, dummyMesh);

    UINT stride = sizeof(Sprite::Vertex);
    UINT offset = 0;
    dc->IASetVertexBuffers(0, 1, skymap_sprite->GetVertexBuffer().GetAddressOf(), &stride, &offset);
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    dc->Draw(4, 0);

    End(rc);
}

// 終了処理
void SkyMap::End(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // シェーダー設定解除
    dc->VSSetShader(nullptr, nullptr, 0);
    dc->PSSetShader(nullptr, nullptr, 0);
    dc->IASetInputLayout(nullptr);

    // 定数バッファ設定解除
    ID3D11Buffer* nullCbs[] = { nullptr };
    dc->VSSetConstantBuffers(8, 1, nullCbs);
    dc->PSSetConstantBuffers(8, 1, nullCbs);

    // サンプラステート設定解除
    ID3D11SamplerState* nullSamplers[] = { nullptr };
    dc->PSSetSamplers(0, 1, nullSamplers);

    // シェーダーリソースビュー設定解除
    ID3D11ShaderResourceView* nullSrvs[] = { nullptr };
    dc->PSSetShaderResources(0, 1, nullSrvs);
}