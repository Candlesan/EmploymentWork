#include "UIFillRenderer.h"
#include "System/Graphic/GpuResourceUtils.h"

// 初期化
void UIFillRenderer::Initialize(ID3D11Device* device)
{
    // ピクセルシェーダー作成
    GpuResourceUtils::LoadPixelShader(
        device,
        "Data/Shader/UIFillPS.cso",
        pixelShader.ReleaseAndGetAddressOf());

    // 定数バッファ作成
    GpuResourceUtils::CreateConstantBuffer(
        device,
        sizeof(UIFillConstants),
        constantBuffer.ReleaseAndGetAddressOf());
}

// CB更新＆バインド
void UIFillRenderer::Apply(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4& uvRect, float fillAmount, FillMethod fillMethod,
    ID3D11ShaderResourceView* mask, float highlight, float time, const DirectX::XMFLOAT4& glowColor)
{
    UIFillConstants constants{};
    constants.uvRect = uvRect;
    constants.fillAmount = fillAmount;
    constants.fillMethod = static_cast<int>(fillMethod);
    constants.highlight = highlight;
    constants.time = time;
    constants.glowColor = glowColor;
    constants.useMask = mask ? 1 : 0;
    dc->UpdateSubresource(constantBuffer.Get(), 0, nullptr, &constants, 0, 0);
    dc->PSSetConstantBuffers(9, 1, constantBuffer.GetAddressOf());

    // maskがないときはnullを入れる
    dc->PSSetShaderResources(1, 1, &mask);
}
