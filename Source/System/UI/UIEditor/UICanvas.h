#pragma once
#include "System/UI/UIEditor/UITween.h"
#include "System/Renderer/UIFillRenderer/UIFillRenderer.h"
#include "json.hpp"

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

	UITween<float> fadeAnim;				// フェードのアニメーション
	float fadeAlpha = 1.0f;					// フェード用の倍率

	UITween<DirectX::XMFLOAT2> moveAnim;	// 移動のアニメーション
	DirectX::XMFLOAT2 animPosition = { 0.0f, 0.0f };

	UITween<float> rotationAnim;			// 回転のアニメーション
	float rotationValue = 0.0f;

	UITween<float> scaleAnim;				// 拡大のアニメーション
	float scaleValue = 1.0f;

	FillMethod fillMethod = FillMethod::None;
	float fillAmount;
};

struct Canvas
{
	DirectX::XMFLOAT2 referenceResolution = { 1920.0f, 1080.0f }; // 基準解像度
	std::vector<std::unique_ptr<UIElement>> elements;
};

class UICanvas
{
public:
	// 初期化
	void Initialize(ID3D11Device* device);

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(ID3D11DeviceContext* dc);

	// GUI描画
	void DrawGUI();

	// 再生
	void Play(UIElement* e);

	// UIを全て再生
	void AllPlay();

	// 停止
	void Stop(UIElement* e);

	// 再生中か
	bool IsPlaying(UIElement* e);

	// 保存
	void Save(const std::string& path);

	// 読み込み
	void Load(const std::string& path);

	// Elementを返す関数
	std::vector<std::unique_ptr<UIElement>>& GetElements() { return canvas.elements; }

	// UIを探す関数
	UIElement* Find(const std::string& name);
	UIElement* FindElement(UIElement* e, const std::string& name);

	// 要素のアニメーションを全部止めて、見た目を元に戻す
	void ResetAnimation(UIElement* e);

	std::shared_ptr<Sprite> CreateSprite(const std::string& path);
private:
	void UpdateElement(UIElement* e, float elapsedTime);
	void RenderElement(ID3D11DeviceContext* dc, UIElement* e, const DirectX::XMMATRIX& parentWorld, const DirectX::XMFLOAT4& parentColor);

	nlohmann::json SaveElement(const UIElement* e);
	std::unique_ptr<UIElement> LoadElement(const nlohmann::json& je, UIElement* parent);

private:
	ID3D11Device* device = nullptr; // デバイス保存用変数
	Canvas canvas; 
	UIFillRenderer uifillRenderer;
};
