#include "SkyMap.h"
#include "Misc.h"
#include "GpuResourceUtils.h"
#include "Graphics.h"


// コンストラクタ
SkyMap::SkyMap(ID3D11Device* device)
{
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

	// メッシュ用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(SkymapConstns),
		skymap_constant_buffer.GetAddressOf());

	// テクスチャ読み込み
	GpuResourceUtils::LoadTexture(
		device,
		"Data/Sprite/SkyMap/Sky.png",
		//"Data/Sprite/SkyMap/night.png",
		skymap_shader_resource_view.GetAddressOf(),
		&skymap_texture2d_desc);

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

	// 定数バッファ設定
	ID3D11Buffer* cbs[] =
	{
		skymap_constant_buffer.Get(),
	};
	dc->PSSetConstantBuffers(8, _countof(cbs), cbs);
}

// 更新処理
void SkyMap::Update(const RenderContext& rc, const Model::Mesh& mesh)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// 定数バッファ更新
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	V.r[3] = DirectX::XMVectorSet(0, 0, 0, 1);

	SkymapConstns skymap{};
	DirectX::XMStoreFloat4x4(&skymap.inverse_view_projection, DirectX::XMMatrixInverse(nullptr, V * P));
	dc->UpdateSubresource(skymap_constant_buffer.Get(), 0, 0, &skymap, 0, 0);

	// シェーダーリソースビュー設定
	ID3D11ShaderResourceView* srvs[] =
	{
		skymap_shader_resource_view.Get()
	};
	dc->PSSetShaderResources(0, _countof(srvs), srvs);
}

void SkyMap::Draw(const RenderContext& rc)
{
	// 1. 行列計算とバッファ更新（これを忘れると何も送られない）
	Model::Mesh dummyMesh{};
	this->Update(rc, dummyMesh);

	// 2. パイプライン設定
	ID3D11DeviceContext* dc = rc.deviceContext;
	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	// 3. 定数バッファを b8 にセット
	ID3D11Buffer* cbs[] = { skymap_constant_buffer.Get() };
	dc->VSSetConstantBuffers(8, 1, cbs);
	dc->PSSetConstantBuffers(8, 1, cbs);

	// 4. サンプラー設定
	ID3D11SamplerState* samplers[] = { rc.renderState->GetSamplerState(SamplerState::LinearWrap) };
	dc->PSSetSamplers(0, 1, samplers);

	// 5. 描画
	float w = Graphics::Instance().GetScreenWidth();
	float h = Graphics::Instance().GetScreenHeight();
	skymap_sprite->Render(
		dc, 0, 0, 1.0f, w, h, 0, 1, 1, 1, 1,
		this->pixelShader.Get(),
		this->vertexShader.Get()
	);
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
	ID3D11Buffer* cbs[] = { nullptr };
}