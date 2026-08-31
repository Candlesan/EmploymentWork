#include "meshVFX.h"
#include "System/Graphic/Graphics.h"
#include "System/Graphic/GpuResourceUtils.h"
#include "System/Core/Misc.h"


meshVFX::meshVFX(ID3D11Device* device)
{
	this->device = device;

	// 頂点シェーダー
	GpuResourceUtils::LoadVertexShader(
		device,
		"Data/Shader/meshVFX_vs.cso",
		Model::InputElementDescs.data(),
		static_cast<UINT>(Model::InputElementDescs.size()),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/meshVFX_ps.cso",
		pixelShader.GetAddressOf());

	// メッシュVFX用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(meshVFXCB),
		meshVFXConstantBuffer.GetAddressOf());

	// ダミーテクスチャ作成
	GpuResourceUtils::CreateDummyTexture(
		device,
		0xFFFFFFFF,
		shaderResourceView.GetAddressOf(),
		&trail_texture2d_desc);
}

// 開始
void meshVFX::Begin(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestOnly), 0);

	// 半透明ブレンド
	if (blendMode == 1) // 加算合成 (Additive)
	{
		dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Additive), nullptr, 0xFFFFFFFF);
	}
	else // 通常半透明 (Transparency)
	{
		dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	}

	// 定数バッファ
	ID3D11Buffer* cbs[] =
	{
		meshVFXConstantBuffer.Get(),
	};
	dc->PSSetConstantBuffers(0, _countof(cbs), cbs);
	dc->VSSetConstantBuffers(0, _countof(cbs), cbs);

	// サンプラーステート
	ID3D11SamplerState* samplers[] = { rc.renderState->GetSamplerState(SamplerState::LinearWrap) };
	dc->PSSetSamplers(0, _countof(samplers), samplers);
}

void meshVFX::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&worldTransform); // 自身のワールド座標を使う
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	DirectX::XMMATRIX WVP = W * V * P;

	meshVFXCB meshVFX{};
	DirectX::XMStoreFloat4x4(&meshVFX.worldViewProjection, WVP);
	meshVFX.vfxColor = vfxColor;
	meshVFX.uvOffset = uvOffset;
	meshVFX.noiseUvOffset = noiseUvOffset;
	meshVFX.distortionPower = distortionPower;
	dc->UpdateSubresource(meshVFXConstantBuffer.Get(), 0, nullptr, &meshVFX, 0, 0);

	ID3D11ShaderResourceView* srvs[] = {
		shaderResourceView.Get(),
		noiseShaderResourceView.Get(),
	};
	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}

void meshVFX::End(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	dc->VSSetShader(nullptr, nullptr, 0);
	dc->PSSetShader(nullptr, nullptr, 0);
	dc->IASetInputLayout(nullptr);

	ID3D11Buffer* nullCBs[] = { nullptr };
	dc->VSSetConstantBuffers(0, _countof(nullCBs), nullCBs);
	dc->PSSetConstantBuffers(0, _countof(nullCBs), nullCBs);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr };
	dc->PSSetShaderResources(0, _countof(nullSRVs), nullSRVs);
}