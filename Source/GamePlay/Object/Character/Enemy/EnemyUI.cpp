#include "EnemyUI.h"

#include "System/Core/Mathf.h"

#include "Gameplay/Object/Character/Character.h"
#include "GamePlay/Object/Camera/Camera.h"

#include <imgui.h>
#include <algorithm>


// 初期化
void EnemyUI::Initialize(UICanvas& uiCanvas)
{
	hpBar = uiCanvas.Find("EnemyHP");
	guardBar = uiCanvas.Find("EnemyGuard");
}

// 更新処理
void EnemyUI::Update(const CharacterStatus& status)
{
	// HP・MPなどの現在の数値から最大値までを0～1に直すラムダ関数
	auto setFill = [](UIElement* e, float current, float max)
	{
		if (!e || max <= 0.0f) return; // UIElementが無いまたは最大値が0以下なら処理をしない
		e->fillAmount = std::clamp(current / max, 0.0f, 1.0f); // 0～1に直す
	};

	setFill(hpBar, status.currentHp, status.maxHp);
	setFill(guardBar, status.currentDefenseDurability, status.maxDefenseDurability);
}

// GUI描画
void EnemyUI::DrawGUI()
{
}

// ワールド座標 → UI座標に変換する
bool EnemyUI::WorldToUI(const DirectX::XMFLOAT3& worldPos, const Camera& camera, DirectX::XMFLOAT2& uiPos)
{
	using namespace DirectX;

	// viewとprojectionを1つの行列にまとめる
	XMMATRIX View = XMLoadFloat4x4(&camera.GetView());
	XMMATRIX Projection = XMLoadFloat4x4(&camera.GetProjection());
	XMMATRIX ViewProjection = View * Projection;

	// w = 1 に変換する
	XMVECTOR Pos = XMVectorSet(worldPos.x, worldPos.y, worldPos.z, 1.0f);

	// ワールド→ビュー→クリップ座標
	XMVECTOR Clip = XMVector4Transform(Pos, ViewProjection);

	XMFLOAT4 clip;
	XMStoreFloat4(&clip, Clip);

	// カメラの後ろにある
	if (clip.w <= 0.0f) return false;

	// wで割ってNDC（-1～1）にする
	float ndcX = clip.x / clip.w;
	float ndcY = clip.y / clip.w;

	// NDC→UI座標（画面中央が0）
	uiPos.x = ndcX * 960.0f;
	uiPos.y = -ndcY * 540.0f; // 画面は下に行くほどYの値が増えるから

	return true;
}

