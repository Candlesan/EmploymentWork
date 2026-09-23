#pragma once
#include "System/UI/Dialog/Dialog.h"

#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>

// 前方宣言
class Sprite;

struct UIElement
{
	std::string name = "name";			// UI名(Findとかで探すよう)
	std::string texturePath;	// テクスチャパス
	DirectX::XMFLOAT4 uvRect = { 0.0f, 0.0f, 0.0f, 0.0f };	// 画像の切り抜き範囲

	// Transform
	DirectX::XMFLOAT2 position = { 0.0f, 0.0f };			// 位置(アンカーからの位置)
	float rotation = 0.0f;									// 回転
	DirectX::XMFLOAT2 size = { 100.0f, 100.0f };			// 拡大(幅・高さ)
	DirectX::XMFLOAT2 scale = { 1.0f, 1.0f };				// 拡大率
	DirectX::XMFLOAT2 pivot = { 0.5f, 0.5f };				// 自分の中で回転・拡大の中心となる点
	DirectX::XMFLOAT2 anchor = { 0.5f, 0.5f };				// 親の四角形のどこを基準のするか

	// Color
	DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f };	// 色
	bool visible = true;

	// 階層
	UIElement* parent = nullptr;						// 親
	std::vector<std::unique_ptr<UIElement>> children;	// この並び順がそのまま描画

	// 実行時に使う変数
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4 worldColor;

	// UIElementに持たせることでElementごとに違う画像が出せる
	std::shared_ptr<Sprite> sprite;
};

class UIEditor
{
public:
	UIEditor() = default;
	~UIEditor() = default;

	// 初期化
	void Initialize(ID3D11Device* device);

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(ID3D11DeviceContext* dc);

	// GUI描画
	void DrawGUI();
private:
	// 保存
	void Save(const std::string& path);

	// 読み込み
	void Load(const std::string& path);

	// UI追加
	UIElement* AddElement();

	// テクスチャ設定
	void SetTexture(UIElement& e, const std::string& path);
private:
	std::vector<std::unique_ptr<UIElement>> elements;
	ID3D11Device* device = nullptr;
	int selectedIndex = -1; // 選択中のUI(-1は未選択)
};
