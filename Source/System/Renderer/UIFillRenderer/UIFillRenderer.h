#pragma once
#include <wrl.h>
#include <d3d11.h>
#include <string>
#include <DirectXMath.h>

enum class FillMethod
{
	None,             // なし（通常の全面描画）
	LeftToRight,      // 左 → 右（一般的なHPバーなど）
	RightToLeft,      // 右 → 左（敵のHPバーや対戦UIの逆向きバーなど）
	BottomToTop,      // 下 → 上（縦のゲージ、Heatバーなど）
	TopToBottom,      // 上 → 下（上から垂れ下がるゲージなど）
	Clockwise,		  // 時計回り（円形、スキルCDなど）
	CounterClockwise  // 反時計回り
};

class UIFillRenderer
{
public:
	// 初期化
	void Initialize(ID3D11Device* device);

	// CB更新＆バインド
	void Apply(ID3D11DeviceContext* dc, const DirectX::XMFLOAT4& uvRect, float fillAmount, FillMethod fillMethod,
		ID3D11ShaderResourceView* maskTexture, float highlight, float time, const DirectX::XMFLOAT4& glowColor);

	ID3D11PixelShader* GetPixelShader() const { return pixelShader.Get(); }
private:
	struct UIFillConstants
	{
		DirectX::XMFLOAT4 uvRect;	// 切り抜き範囲
		float fillAmount;			// 塗り割合
		int fillMethod;				// None, 左右, 時計回り
		float highlight;
		float time;
		DirectX::XMFLOAT4 glowColor;
		int useMask;
		DirectX::XMFLOAT3 pad;
	};

	Microsoft::WRL::ComPtr<ID3D11PixelShader>        pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer>			 constantBuffer;
};