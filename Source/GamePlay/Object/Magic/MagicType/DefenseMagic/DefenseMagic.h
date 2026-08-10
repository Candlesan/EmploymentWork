#pragma once
#include <DirectXMath.h>

class Character;

class DefenseMagic
{
public:
	// 初期化
	void Initialize();

	// 更新処理
	void Update(Character* owner, float elapsedTime);

	// 描画処理
	void Render();

	// GUI描画
	void DrawGUI();

	// ゲッター
	DirectX::XMFLOAT3 GetGaurdPosition() const { return gaurdPosition; }
	float GetGaurdRadius() const { return gaurdRadius; }

	// セッター
	void SetIsActive(bool flag) { isActive = flag; }
	bool GetIsActive() const { return isActive; }
private:
	DirectX::XMFLOAT3 gaurdPosition = { 0.0f, 0.0f, 0.0f }; // 防御魔法の位置
	float gaurdRadius = 2.0f; // 防御魔法の半径
	bool isActive = false; // 防御魔法を使っているかいないか
};