#include "Trail.h"
#include "System/Graphic/Graphics.h"
#include "System/Graphic/GpuResourceUtils.h"
#include "System/Core/Misc.h"


// 初期化
void Trail::Initialize()
{
	// デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	HRESULT hr = S_OK;
	// 頂点バッファの生成
	{
		// 頂点バッファを作成するための設定オプション
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = sizeof(TrailVertex) * MAX_POLYGON * 2;
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		// 頂点バッファオブジェクトの生成
		hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	}

	// インプットレイアウト
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
		"Data/Shader/TrailVS.cso",
		inputElementDesc,
		ARRAYSIZE(inputElementDesc),
		inputLayout.GetAddressOf(),
		vertexShader.GetAddressOf());

	// ピクセルシェーダー
	GpuResourceUtils::LoadPixelShader(
		device,
		"Data/Shader/TrailPS.cso",
		pixelShader.GetAddressOf());

	// シーン用用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CBScene),
		sceneConstantBuffer.GetAddressOf());

	// 画像読み込み
	GpuResourceUtils::LoadTexture(
		device,
		"Data/Sprite/Trail/T_TrailMask_09.png",
		shaderResourceView.GetAddressOf(),
		&trail_texture2d_desc);
}

// 更新処理
void Trail::Update(DirectX::XMFLOAT3 root, DirectX::XMFLOAT3 tip)
{
	// 過去の軌跡を一つ後ろにずらす
	for (int i = MAX_POLYGON - 1; i > 0; --i)
	{
		trailPositions[0][i] = trailPositions[0][i - 1]; // 根本を一つ後ろへ
		trailPositions[1][i] = trailPositions[1][i - 1]; // 先端を一つ後ろへ
	}

	// 剣の根元の座標を取得し最新の頂点座標を保存
	trailPositions[0][0] = root;
	trailPositions[1][0] = tip;
}

// 描画処理
void Trail::Render(const RenderContext& rc)
{
	// ポリゴン作成
	//PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	//if (!spline)
	//{
	//	// 保存していた頂点バッファでポリゴンを作る
	//	for (int i = 0; i < MAX_POLYGON; ++i)
	//	{
	//		primitiveRenderer->AddVertex(trailPositions[0][i], color);
	//		primitiveRenderer->AddVertex(trailPositions[1][i], color);
	//	}
	//}
	//else
	//{
	//	const int segments = 8;

	//	for (int i = 1; i < MAX_POLYGON - 2; ++i) // ちゃんとp0～p3が揃う範囲
	//	{
	//		// 根元側と先端側の4点を取得（正しい順番）
	//		DirectX::XMVECTOR rootP0 = DirectX::XMLoadFloat3(&trailPositions[0][i - 1]);
	//		DirectX::XMVECTOR rootP1 = DirectX::XMLoadFloat3(&trailPositions[0][i]);
	//		DirectX::XMVECTOR rootP2 = DirectX::XMLoadFloat3(&trailPositions[0][i + 1]);
	//		DirectX::XMVECTOR rootP3 = DirectX::XMLoadFloat3(&trailPositions[0][i + 2]);

	//		DirectX::XMVECTOR tipP0 = DirectX::XMLoadFloat3(&trailPositions[1][i - 1]);
	//		DirectX::XMVECTOR tipP1 = DirectX::XMLoadFloat3(&trailPositions[1][i]);
	//		DirectX::XMVECTOR tipP2 = DirectX::XMLoadFloat3(&trailPositions[1][i + 1]);
	//		DirectX::XMVECTOR tipP3 = DirectX::XMLoadFloat3(&trailPositions[1][i + 2]);

	//		for (int j = 0; j < segments; ++j)
	//		{
	//			float t = static_cast<float>(j) / segments;

	//			// スプライン補完
	//			DirectX::XMVECTOR rootPos = DirectX::XMVectorCatmullRom(rootP0, rootP1, rootP2, rootP3, t);
	//			DirectX::XMVECTOR tipPos = DirectX::XMVectorCatmullRom(tipP0, tipP1, tipP2, tipP3, t);

	//			DirectX::XMFLOAT3 rootResult, tipResult;
	//			DirectX::XMStoreFloat3(&rootResult, rootPos);
	//			DirectX::XMStoreFloat3(&tipResult, tipPos);

	//			primitiveRenderer->AddVertex(rootResult, color);
	//			primitiveRenderer->AddVertex(tipResult, color);
	//		}
	//	}
	//}

	ID3D11DeviceContext* dc = rc.deviceContext;

	// 頂点書き込み
	D3D11_MAPPED_SUBRESOURCE mapped;
	dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	TrailVertex* v = static_cast<TrailVertex*>(mapped.pData);

	for (int i = 0; i < MAX_POLYGON; ++i)
	{
		float uvV = (float)i / (MAX_POLYGON - 1); // 0が新しい端、1が古い端

		v[i * 2 + 0].position = trailPositions[0][i]; // 根本
		v[i * 2 + 0].color = color;
		v[i * 2 + 0].texcoord = { uvV, 0.0f };

		v[i * 2 + 1].position = trailPositions[1][i]; // 根本
		v[i * 2 + 1].color = color;
		v[i * 2 + 1].texcoord = { uvV, 1.0f };
	}
	dc->Unmap(vertexBuffer.Get(), 0);

	ID3D11SamplerState* samplers[] = {
	rc.renderState->GetSamplerState(SamplerState::LinearWrap)
	};
	dc->PSSetSamplers(0, 1, samplers);

	// シェーダーをセット
	dc->IASetInputLayout(inputLayout.Get());
	dc->VSSetShader(vertexShader.Get(), nullptr, 0);
	dc->PSSetShader(pixelShader.Get(), nullptr, 0);

	CBScene cbScene{};
	DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
	DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
	DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);
	rc.deviceContext->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);

	rc.deviceContext->VSSetConstantBuffers(7, 1, sceneConstantBuffer.GetAddressOf());

	UINT stride = sizeof(TrailVertex);
	UINT offset = 0;
	dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
	dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	dc->OMSetDepthStencilState(
		rc.renderState->GetDepthStencilState(DepthState::TestOnly),
		0);

	dc->OMSetBlendState(
		rc.renderState->GetBlendState(BlendState::Additive), // 加算ブレンド
		nullptr, 0xFFFFFFFF);

	// 描画
	dc->Draw(MAX_POLYGON * 2, 0);

	dc->OMSetDepthStencilState(
		rc.renderState->GetDepthStencilState(DepthState::TestAndWrite),
		0);

	// 描画後に元に戻す
	dc->OMSetBlendState(
		rc.renderState->GetBlendState(BlendState::Opaque),
		nullptr, 0xFFFFFFFF);
}
