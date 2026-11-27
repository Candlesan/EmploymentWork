#pragma once
#include "System/UI/UIEditor/UITween.h"
#include "System/Renderer/UIFillRenderer/UIFillRenderer.h"
#include "System/Core/Input/GamePad.h"
#include "json.hpp"

#include <string>
#include <vector>
#include <memory>
#include <d3d11.h>
#include <DirectXMath.h>

// 前方宣言
class Sprite;

// アニメーション情報
struct UIAnimSet
{
	UITween<float> fade;				// フェードアニメーション
	UITween<DirectX::XMFLOAT2> move;	// 移動アニメーション
	UITween<float> rotation;			// 回転アニメーション
	UITween<float> scale;				// 拡縮アニメーション

	// 再生
	void Play() { fade.Play(); move.Play(); rotation.Play(); scale.Play(); }

	// 停止
	void Stop() { fade.Stop(); move.Stop(); rotation.Stop(); scale.Stop(); }

	// 再生中か
	bool IsPlaying() const { return fade.playing || move.playing || rotation.playing || scale.playing; }
};

struct UIElement
{
	std::string name = "name";			// UI名(Findとかで探すよう)
	std::string texturePath;			// テクスチャパス
	std::string maskTexturePath;		// マスク用のテクスチャパス
	DirectX::XMFLOAT4 uvRect = { 0.0f, 0.0f, 0.0f, 0.0f };	// 画像の切り抜き範囲
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> maskSRV;

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

	UIAnimSet anim; // UIアニメーション
	float fadeAlpha = 1.0f;
	DirectX::XMFLOAT2 animPosition = { 0.0f, 0.0f };
	float rotationValue = 0.0f;
	float scaleValue = 1.0f;

	// 塗りつぶし
	FillMethod fillMethod = FillMethod::None;
	float fillAmount = 1.0f;

	// ハイライト
	float highlight = 0.0f; // 今の値
	float highlightTarget = 0.0f; // 目標の値
	DirectX::XMFLOAT4 glowColor = { 1, 1, 1, 1 };


	// フォーカス
	UIAnimSet focusAnim; // 選ばれたとき
	UIAnimSet unfocusAnim; // 外れたとき

	int  focusIndex = -1;    // -1: 選択不可, 0以上: 選択順
	bool focusHorizontal = false; // パネル用: false=縦, true=横
	bool focusLoop = false; // パネル用: 端で先頭に戻るか
};

struct Canvas
{
	DirectX::XMFLOAT2 referenceResolution = { 1920.0f, 1080.0f }; // 基準解像度
	std::vector<std::unique_ptr<UIElement>> elements;
};

// 選んでいるUI
struct FocusGroup
{
	UIElement* owner = nullptr; // 集める元
	std::vector<UIElement*> items; // 選べる物
	int pos = -1; // 現在選んでいるもの

	UIElement* Focused() const
	{
		return (pos >= 0 && pos < static_cast<int>(items.size())) ? items[pos] : nullptr;
	}

	void Clear() { owner = nullptr; items.clear(); pos = -1; }
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

	// スプライトを作る
	std::shared_ptr<Sprite> CreateSprite(const std::string& path);

	// 
	void SetPanel(const std::string& name); // どのパネルを選ぶか決めて先頭にフォーカスする
	void UpdateNavigation(const GamePad& pad); // 入力を集める
	UIElement* GetFocused() { return tabFocus.Focused(); }
	UIElement* GetSubFocused() { return itemFocus.Focused(); }

	int GetFocusIndex() const { return tabFocus.pos; }
	int GetSubFocusIndex() const { return itemFocus.pos; }
private:
	void UpdateElement(UIElement* e, float elapsedTime);
	void RenderElement(ID3D11DeviceContext* dc, UIElement* e, const DirectX::XMMATRIX& parentWorld, const DirectX::XMFLOAT4& parentColor);

	nlohmann::json SaveElement(const UIElement* e);
	std::unique_ptr<UIElement> LoadElement(const nlohmann::json& je, UIElement* parent);

	void Navigate(int dx, int dy);								// 入力方向に動かす
	void Collect(FocusGroup& g, UIElement* owner);				// ownerの子から選べる物を集める
	void Select(FocusGroup& g, int newPos);					// ハイライトを付け替えて位置を変える
	void SelectTab(int newPos);									// タブを変えた時に中の物を集め直す
	void SetHighlight(UIElement* e, bool on);					// ハイライトを切り替える
private:
	ID3D11Device* device = nullptr; // デバイス保存用変数
	Canvas canvas; 
	UIFillRenderer uifillRenderer;

	FocusGroup tabFocus;
	FocusGroup itemFocus;
	bool stickNeutral = true; // スティックを倒していない
	float time = 0.0f; // 経過時間
};
