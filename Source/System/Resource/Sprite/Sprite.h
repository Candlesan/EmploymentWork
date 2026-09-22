#pragma once

#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>

// スプライト
class Sprite
{
public:
	Sprite(ID3D11Device* device);
	Sprite(ID3D11Device* device, const char* filename);
	Sprite(ID3D11Device* device, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shader_resource_view);

	// 頂点データ
	struct Vertex
	{
		DirectX::XMFLOAT3	position;
		DirectX::XMFLOAT4	color;
		DirectX::XMFLOAT2	texcoord;
	};

	// 更新処理
	void Update(
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float sx, float sy,					// 画像切り抜き位置
		float sw, float sh,					// 画像切り抜きサイズ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;

	// 描画実行
	void Render(ID3D11DeviceContext* dc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float sx, float sy,					// 画像切り抜き位置
		float sw, float sh,					// 画像切り抜きサイズ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;

	// 描画実行（テクスチャ切り抜き指定なし）
	void Render(ID3D11DeviceContext* dc,
		float dx, float dy,					// 左上位置
		float dz,							// 奥行
		float dw, float dh,					// 幅、高さ
		float angle,						// 角度
		float r, float g, float b, float a	// 色
	) const;

	// 描画実行（UIEditor用の4点(左上、右上、左下、右下)をスクリーン座標で渡して描く）
	void Render(ID3D11DeviceContext* dc,
		const DirectX::XMFLOAT2 corners[4], // 四隅の座標
		float dz,
		float sx, float sy, float sw, float sh, // 切り抜き（px）
		const DirectX::XMFLOAT4& color) const;

	// テクスチャサイズ取得
	DirectX::XMFLOAT2 GetTextureSize() const { return { textureWidth, textureHeight }; }

	//　外部からシェーダーを変更する関数
	void SetSpriteVS(Microsoft::WRL::ComPtr<ID3D11VertexShader> vs) { vertexShader = vs; }
	void SetSpritePS(Microsoft::WRL::ComPtr<ID3D11PixelShader> ps) { pixelShader = ps; }

	Microsoft::WRL::ComPtr<ID3D11Buffer> GetVertexBuffer() { return vertexBuffer; }
private:
	// 4頂点NDCに変換して頂点バッファに書き込む
	void WriteVertices(ID3D11DeviceContext* dc,
		const DirectX::XMFLOAT2 corners[4], // 四隅の座標
		float dz,
		float sx, float sy, float sw, float sh, // 切り抜き（px）
		const DirectX::XMFLOAT4& color) const;

	// シェーダー等をセットして描画
	void Draw(ID3D11DeviceContext* dc) const;
private:
	// デフォルトシェーダー
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>			inputLayout;

	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	shaderResourceView;
	D3D11_TEXTURE2D_DESC texture2d_desc;

	float textureWidth = 0;
	float textureHeight = 0;

	bool isLoadFile = true;	//	UNIT.09
};
