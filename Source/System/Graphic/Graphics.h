#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <memory>
#include "RenderState.h"
#include "System/Renderer/PrimitiveRenderer.h"
#include "System/Renderer/ShapeRenderer.h"
#include "System/Renderer/ModelRenderer.h"

// シェーダー
#include "System/Renderer/Shader/ShadowMap/ShadowMap.h"

// グラフィックス
class Graphics
{
private:
	Graphics() = default;
	~Graphics() = default;
	//~Graphics()
	//{
	//	shadowMap.reset();
	//	modelRenderer.reset();
	//	shapeRenderer.reset();
	//	primitiveRenderer.reset();
	//	renderState.reset();

	//	depthStencilView.Reset();
	//	renderTargetView.Reset();

	//	if (immediateContext)
	//	{
	//		immediateContext->ClearState();
	//		immediateContext->Flush();
	//	}
	//	immediateContext.Reset();
	//	swapchain.Reset();

	//	 ← ここに一時的に追加
	//	OutputDebugStringA("=== ReportLiveDeviceObjects 呼び出し ===\n");

	//	Microsoft::WRL::ComPtr<ID3D11Debug> d3dDebug;
	//	HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&d3dDebug));

	//	 ← hrの結果も出力
	//	char buf[128];
	//	sprintf_s(buf, "QueryInterface hr = 0x%08X\n", hr);
	//	OutputDebugStringA(buf);

	//	if (SUCCEEDED(hr))
	//	{
	//		d3dDebug->ReportLiveDeviceObjects(
	//			D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL
	//		);
	//	}

	//	device.Reset();
	//}

public:
	// インスタンス取得
	static Graphics& Instance()
	{
		static Graphics instance;
		return instance;
	}

	// 初期化
	void Initialize(HWND hWnd);

	// クリア
	void Clear(float r, float g, float b, float a);

	// レンダーターゲット設定
	void SetRenderTargets();

	// 画面表示
	void Present(UINT syncInterval);

	// ウインドウハンドル取得
	HWND GetWindowHandle() { return hWnd; }

	// デバイス取得
	ID3D11Device* GetDevice() { return device.Get(); }

	// デバイスコンテキスト取得
	ID3D11DeviceContext* GetDeviceContext() { return immediateContext.Get(); }

	// スクリーン幅取得
	float GetScreenWidth() const { return screenWidth; }

	// スクリーン高さ取得
	float GetScreenHeight() const { return screenHeight; }

	// レンダーステート取得
	RenderState* GetRenderState() { return renderState.get(); }

	// プリミティブレンダラ取得
	PrimitiveRenderer* GetPrimitiveRenderer() const { return primitiveRenderer.get(); }

	// シェイプレンダラ取得
	ShapeRenderer* GetShapeRenderer() const { return shapeRenderer.get(); }

	// モデルレンダラ取得
	ModelRenderer* GetModelRenderer() const { return modelRenderer.get(); }

	// シャドウマップ取得
	ShadowMap* GetShadowMap() const { return shadowMap.get(); }

private:
	HWND hWnd = nullptr;
	float screenWidth = 0;
	float screenHeight = 0;
	D3D11_VIEWPORT viewport;

	// ↓ Renderer系を先に宣言（先に解放される）
	std::unique_ptr<ShadowMap>          shadowMap;
	std::unique_ptr<ModelRenderer>      modelRenderer;
	std::unique_ptr<ShapeRenderer>      shapeRenderer;
	std::unique_ptr<PrimitiveRenderer>  primitiveRenderer;
	std::unique_ptr<RenderState>        renderState;

	// ↓ D3D11リソース（後に解放される）
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  depthStencilView;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  renderTargetView;
	Microsoft::WRL::ComPtr<IDXGISwapChain>          swapchain;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>      immediateContext;
	Microsoft::WRL::ComPtr<ID3D11Device>            device; // 一番最後
};
