#pragma once
#include <directXMath.h>
#include <d3d11.h>
#include <wrl.h>
#include "RenderContext.h"

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, UINT width = 4096, UINT height = 4096);

	// 切り替え関数
	void Activate(ID3D11DeviceContext* dc);	// シャドウマップに切り替え
	//void Activate(ID3D11DeviceContext* dc, int cascadeIndex); // カスケードシャドウマップ用
	void Deactivate(ID3D11DeviceContext* dc); // 元に戻す

	// ゲッター
	ID3D11ShaderResourceView* GetShaderResourceView() const { return shaderResourceView.Get(); }
	ID3D11SamplerState* GetSamplerState() const { return samplerState.Get(); }

	UINT GetWidth() const { return width; }
	UINT GetHeight() const { return height; }
private:
	//Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView; // カスケード用
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;

	D3D11_VIEWPORT viewport;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthBuffer;

	// 元の状態保存用
	D3D11_VIEWPORT cachedViewport;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> cachedRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> cachedDSV;

	UINT width, height;
};
