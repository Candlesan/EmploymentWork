// BLOOM
#pragma once

#include <memory>
#include <d3d11.h>
#include <wrl.h>

#include "framebuffer.h"
#include "fullscreen_quad.h"
#include "RenderState.h"

class bloom
{
public:
	bloom(ID3D11Device* device, uint32_t width, uint32_t height);
	~bloom() = default;
	bloom(const bloom&) = delete;
	bloom& operator =(const bloom&) = delete;
	bloom(bloom&&) noexcept = delete;
	bloom& operator =(bloom&&) noexcept = delete;

	void bloom::make(ID3D11DeviceContext* immediate_context, ID3D11ShaderResourceView* color_map, RenderState* render_state);
	ID3D11ShaderResourceView* shader_resource_view() const
	{
		return glow_extraction->shader_resource_views[0].Get();
	}

	// シーンゲームで定数バッファを設定する関数
	void set_constant_buffer(ID3D11DeviceContext* immediate_context);

	// デバック用
	ID3D11ShaderResourceView* get_glkow_extraction() const
	{
		return glow_extraction->shader_resource_views[0].Get();
	}
	ID3D11ShaderResourceView* get_gaussian_blur(size_t index, size_t pass) const
	{
		return gaussian_blur[index][pass]->shader_resource_views[0].Get();
	}

public:
	float bloom_extraction_threshold = 0.57f;
	float bloom_intensity = 1.0f;
	float exposure = 1.0;
	static const size_t downsampled_count = 3;

private:
	std::unique_ptr<fullscreen_quad> bit_block_transfer;
	std::unique_ptr<framebuffer> glow_extraction;

	std::unique_ptr<framebuffer> gaussian_blur[downsampled_count][2];

	Microsoft::WRL::ComPtr<ID3D11PixelShader> glow_extraction_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_downsampling_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_horizontal_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_vertical_ps;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> gaussian_blur_upsampling_ps;

	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depth_stencil_state;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer_state;
	Microsoft::WRL::ComPtr<ID3D11BlendState> blend_state;

	struct bloom_constants
	{
		float bloom_extraction_threshold;
		float bloom_intensity;
		float exposure;
		float something;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> constant_buffer;
};
