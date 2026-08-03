#pragma once

#include "System/Renderer/Shader/Shader.h"

class meshVFX : public Shader
{
public:
	meshVFX(ID3D11Device* device);
	~meshVFX() override = default;

	// 開始処理
	void Begin(const RenderContext& rc) override;

	// 更新処理
	void Update(const RenderContext& rc, const Model::Mesh& mesh) override;

	// 終了処理
	void End(const RenderContext& rc) override;

	void SetBlendMode(int mode) { blendMode = mode; }

	// ゲッターセッター
	void SetColor(DirectX::XMFLOAT4 c) { vfxColor = c; }
	void SetUvOffset(DirectX::XMFLOAT2 offset) { uvOffset = offset; }
	void SetNoiseUvOffset(DirectX::XMFLOAT2 offset) { noiseUvOffset = offset; } 
	void SetDistortionPower(float power) { distortionPower = power; } 

	void SetCurrentTextures(ID3D11ShaderResourceView* srv, ID3D11ShaderResourceView* noiseSrv)
	{
		shaderResourceView = srv;
		noiseShaderResourceView = noiseSrv;
	}

private:
	// シーン定数バッファ
	struct CBScene
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT4 lightDirection; // 使わなくても必要！
		DirectX::XMFLOAT4 lightColor;     // 使わなくても必要！
		DirectX::XMFLOAT4 cameraPosition; // 使わなくても必要！
		DirectX::XMFLOAT4X4 inverseView;
	};

	struct meshVFXCB
	{
		DirectX::XMFLOAT4X4 worldViewProjection; // WVP行列
		DirectX::XMFLOAT4   vfxColor;            // メインカラー
		DirectX::XMFLOAT2   uvOffset;            // UVオフセット
		DirectX::XMFLOAT2   noiseUvOffset;	     // 歪みのオフセット
		float               distortionPower;	 // 歪みの強さ
		float               padding[3];			// 16バイト境界のアライメント調整
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader>		vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>		pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>		inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			meshVFXConstantBuffer;

	D3D11_TEXTURE2D_DESC trail_texture2d_desc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noiseShaderResourceView;

	DirectX::XMFLOAT4 vfxColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT2 uvOffset = { 0.0f, 0.0f };

	ID3D11Device* device = nullptr; // テクスチャ再ロード用に保持

	DirectX::XMFLOAT2 noiseUvOffset = { 0.0f, 0.0f };
	float distortionPower = 0.0f;    
	int blendMode = 0;
};