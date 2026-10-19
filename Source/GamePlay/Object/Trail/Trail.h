#pragma once
#include "System/Graphic/RenderContext.h"

#include <DirectXMath.h>
#include <wrl.h>
#include <string>

class Trail
{
public:
	// 初期化
	void Initialize();

	// 更新処理
	void Update(DirectX::XMFLOAT3 root, DirectX::XMFLOAT3 tip, float elapsedTime, float length);

	// 描画処理
	void Render(const RenderContext& rc);

	// ゲッターセッター
	bool SetTexture(const std::string& texturePath);
	void SetColor(DirectX::XMFLOAT4 c) { color = c; }
	void SetBlendMode(int mode) { blendMode = mode; }

	// 前・中・後ろの倍率をセットする関数
	void SetWidthScales(float start, float middle, float end) {
		widthStart = start;
		widthMiddle = middle;
		widthEnd = end;
	}

	void Reset(DirectX::XMFLOAT3 root, DirectX::XMFLOAT3 tip);
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

	static const int MAX_POLYGON = 128;
	DirectX::XMFLOAT3 trailPositions[2][MAX_POLYGON];
	bool spline;
	DirectX::XMFLOAT4 color = { 1, 0, 0, 1 };

	ID3D11Device* device = nullptr; // テクスチャ再ロード用に保持

	// 倍率を保持する変数
	float widthStart = 1.0f;
	float widthMiddle = 1.0f;
	float widthEnd = 0.0f;

	float sampleTimer = 0.0f; // 前の点を記録してからの時間
	int blendMode = 1; // ０：半透明、　１：加算

	float pointAge[MAX_POLYGON] = {};      // 各点が何秒前に記録されたか
	float trailLength = 0.0f;              // Render で使うので覚えておく

	std::string currentTexturePath;
};