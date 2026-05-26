#pragma once
#include "System/Graphic/RenderContext.h"

#include <DirectXMath.h>
#include <wrl.h>

class Trail
{
public:
	// 初期化
	void Initialize();

	// 更新処理
	void Update(DirectX::XMFLOAT3 root, DirectX::XMFLOAT3 tip);

	// 描画処理
	void Render(const RenderContext& rc);
private:
	// シーン定数バッファ
	struct CBScene
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT4 lightDirection; // 使わなくても必要！
		DirectX::XMFLOAT4 lightColor;     // 使わなくても必要！
		DirectX::XMFLOAT4 cameraPosition; // 使わなくても必要！
	};

	struct TrailVertex
	{
		DirectX::XMFLOAT3 position; // 位置
		DirectX::XMFLOAT4 color; // 色
		DirectX::XMFLOAT2 texcoord; // UV座標
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
	D3D11_TEXTURE2D_DESC trail_texture2d_desc;

	static const int MAX_POLYGON = 16;
	DirectX::XMFLOAT3 trailPositions[2][MAX_POLYGON];
	bool spline;
	DirectX::XMFLOAT4 color = { 1, 0, 0, 1 };
};