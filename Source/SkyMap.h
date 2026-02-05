#pragma once
#include "Shader.h"
#include "Sprite.h"
#include <memory>

class SkyMap : public Shader
{
public:
	SkyMap(ID3D11Device* device);
	~SkyMap() override = default;

	// 開始処理
	void Begin(const RenderContext& rc) override;

	// 更新処理
	void Update(const RenderContext& rc, const Model::Mesh& mesh) override;

	// 描画処理
	void Draw(const RenderContext& rc);

	// 終了処理
	void End(const RenderContext& rc) override;

private:
	struct SkymapConstns
	{
		DirectX::XMFLOAT4X4 inverse_view_projection;
	};

	Microsoft::WRL::ComPtr<ID3D11VertexShader>       vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>        pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>        inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			 skymap_constant_buffer;
	D3D11_TEXTURE2D_DESC						     skymap_texture2d_desc;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skymap_shader_resource_view;
	std::unique_ptr<Sprite>							 skymap_sprite;

};
