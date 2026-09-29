#pragma once
#include "json.hpp"
#include "System/Core/Mathf.h"
#include "System/UI/Dialog/Dialog.h"
#include "System/UI/UIEditor/UITween.h"
#include "System/UI/UIEditor/UICanvas.h"

#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>

// 前方宣言
class Sprite;

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

private: // 見た目の部分
	void DrawToolBar();
	void DrawNode();
	void DrawTreeNode(UIElement* e);
	void DrawInspector();

private:

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
	UICanvas uiCanvas; // 編集中のUI
	UIElement* selected = nullptr; // 選択中のUI(未選択はnullptr)
};
