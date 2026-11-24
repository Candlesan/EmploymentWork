#include "Compute_Particle.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"

Compute_Particle::Compute_Particle(ID3D11Device* device, int maxParticle) : device(device)
{
    //	総パーティクル数

    D3D11_TEXTURE2D_DESC texture2d_desc;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view;

    // デフォルトテクスチャの読み込み
    shader_resource_view = TextureResourceManager::Instance().GetOrLoad(device, "Data/Sprite/Particle/texture/particle256x256.png");

    //	コンピュートパーティクルシステム生成
    system = std::make_unique<compute_particle_system>(device,
        maxParticle,
        shader_resource_view,
        DirectX::XMUINT2(4, 4));

    // シーン用用定数バッファ
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(CBScene),
        sceneConstantBuffer.GetAddressOf());
}

void Compute_Particle::Update(ID3D11DeviceContext* dc, DirectX::XMFLOAT3 parent_delta, float elapsedTime)
{
    // 奥行きソート用にカメラの行列を Compute Shader へ渡す
    dc->CSSetConstantBuffers(7, 1, sceneConstantBuffer.GetAddressOf());

    system->update(dc, parent_delta, elapsedTime);
}

void Compute_Particle::Render(const RenderContext& rc)
{
    ID3D11DeviceContext* dc = rc.deviceContext;

    // サンプラーをセット
    ID3D11SamplerState* samplers[] =
    {
        rc.renderState->GetSamplerState(SamplerState::PointWrap),
        rc.renderState->GetSamplerState(SamplerState::LinearWrap),
        rc.renderState->GetSamplerState(SamplerState::AnisotropicWrap)
    };
    dc->PSSetSamplers(0, 3, samplers);

    // 定数バッファ更新
    CBScene cbScene{};
    DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
    DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
    DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);

    DirectX::XMMATRIX invV = DirectX::XMMatrixInverse(nullptr, V);
    DirectX::XMStoreFloat4x4(&cbScene.inverseView, invV);

    dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, nullptr, &cbScene, 0, 0);

    dc->GSSetConstantBuffers(7, 1, sceneConstantBuffer.GetAddressOf());
    dc->VSSetConstantBuffers(7, 1, sceneConstantBuffer.GetAddressOf());


    dc->OMSetDepthStencilState(
        rc.renderState->GetDepthStencilState(DepthState::TestOnly),
        0);

    // ブレンドモードの設定
    // 半透明ブレンド
    if (blendMode == 1) // 加算合成 (Additive)
    {
        dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);
    }
    else // 通常半透明 (Transparency)
    {
        dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    }
    dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));

    // 描画
    system->render_trail(dc); // 帯を先に
    system->render(dc);

    dc->OMSetDepthStencilState(
        rc.renderState->GetDepthStencilState(DepthState::TestAndWrite),
        0);

    // 描画後に元に戻す
    dc->OMSetBlendState(
        rc.renderState->GetBlendState(BlendState::Opaque),
        nullptr, 0xFFFFFFFF);

}

void Compute_Particle::Clear(ID3D11DeviceContext* dc)
{
    if (system) system->clear(dc);
}

bool Compute_Particle::SetTexture(const std::string& texturePath, DirectX::XMUINT2 splitCount)
{
    if (!device || texturePath.empty()) return false;

    D3D11_TEXTURE2D_DESC texture2d_desc;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;

    try
    {
        // 画像を読み込む
        srv = TextureResourceManager::Instance().GetOrLoad(device, texturePath);
        if (!srv) return false;
    }
    catch (...)
    {
        return false; // パスがおかしいときにクラッシュしないようにする
    }


    // システム側のSRVを上書き
    if (system)
    {
        system->set_texture(srv, splitCount);
    }

    return true;
}
