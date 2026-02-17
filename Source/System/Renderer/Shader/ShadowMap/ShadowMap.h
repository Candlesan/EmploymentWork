#pragma once
#include "System/Graphic/RenderContext.h"
#include <directXMath.h>
#include <d3d11.h>
#include <wrl.h>

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, UINT width = 2048, UINT height = 2048);
	~ShadowMap() {}

	// 切り替え関数
	void Active(ID3D11DeviceContext* dc); // シャドウマップに切り替える

	// 元に戻す関数
	void Deactive(ID3D11DeviceContext* dc); // 保存していたものを元に戻す

	// ゲッター
	ID3D11ShaderResourceView* GetShaderResourceView() const { return shaderResourceView.Get(); }
	ID3D11SamplerState* GetSamplerState() const { return samplerState.Get(); }

	UINT GetWidth() const { return width; }
	UINT GetHeight() const { return height; }
private:
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

	D3D11_VIEWPORT viewport;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer;

	// 元の状態保存用
	D3D11_VIEWPORT cachedViewport;
	//Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cachedRTV;
	//Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cachedDSV;

	UINT width, height;
};