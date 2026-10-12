#pragma once
#include "System/UI/UIEditor/UICanvas.h"

// 前方宣言
class Camera;
struct CharacterStatus;

class EnemyUI
{
public:
	// 使うUIをここで先に探しておく
	void Initialize(UICanvas& uiCanvas);

	// UIを更新する
	void Update(const CharacterStatus& status);

	// GUI描画
	void DrawGUI();
private:
	// ワールド座標 → UI座標に変換する
	// 画面に映っていれば true を返し、uiPos に中央からのずれを入れる
	bool WorldToUI(const DirectX::XMFLOAT3& worldPos, const Camera& camera, DirectX::XMFLOAT2& uiPos);
private:
	UIElement* hpBar = nullptr;
	UIElement* guardBar = nullptr;
};
