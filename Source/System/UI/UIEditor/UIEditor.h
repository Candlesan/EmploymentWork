#pragma once
#include "System/UI/Dialog/Dialog.h"
#include "System/Core/Mathf.h"
#include "System/UI/UIEditor/UITween.h"
#include "json.hpp"

#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>

// 前方宣言
class Sprite;

// UIの要素
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
	DirectX::XMFLOAT2 pivot = { 0.0f, 0.0f };				// 自分の中で回転・拡大の中心となる点
	DirectX::XMFLOAT2 anchor = { 0.0f, 0.0f };				// 親の四角形のどこを基準のするか

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

	float fadeAlpha = 1.0f; // フェード用の倍率
	UITween<float> fade; // フェードのアニメーション
};

// キャンバス
struct Canvas
{
	DirectX::XMFLOAT2 referenceResolution = { 1920.0f, 1080.0f }; // 基準解像度
	std::vector<std::unique_ptr<UIElement>> elements;
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
	void UpdateElement(UIElement* e, float elapsedTime);

	// 描画処理
	void Render(ID3D11DeviceContext* dc);
	void RenderElement(ID3D11DeviceContext* dc, UIElement* e, const DirectX::XMMATRIX& parentWorld, const DirectX::XMFLOAT4& parentColor);

	// GUI描画
	void DrawGUI();

private: // 見た目の部分
	void DrawToolBar();
	void DrawNode();
	void DrawTreeNode(UIElement* e);
	void DrawInspector();

private:
	// 保存
	void Save(const std::string& path);
	nlohmann::json SaveElement(const UIElement* e);

	// 読み込み
	void Load(const std::string& path);
	std::unique_ptr<UIElement> LoadElement(const nlohmann::json& je, UIElement* parent);

	// UI追加
	UIElement* AddElement();

	// 子を追加
	UIElement* AddChild(UIElement* parent);

	// テクスチャ設定
	void SetTexture(UIElement& e, const std::string& path);

	// Elementの移動
	void MoveElement(UIElement* e, int direction);

	// 要素が入っている配列(兄弟の配列)を返す
	std::vector<std::unique_ptr<UIElement>>& GetSiblings(UIElement* e);

private:
	ID3D11Device* device = nullptr; // デバイス保存用変数
	Canvas canvas; // 編集中のUI
	UIElement* selected = nullptr; // 選択中のUI(未選択はnullptr)
};
