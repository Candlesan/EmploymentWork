#pragma once
#include <DirectXMath.h>
#include <string>

class Character;

class DefenseMagic
{
public:
	// 初期化
	void Initialize(Character* owner);

	// 更新処理
	void Update(Character* owner, float elapsedTime);

	// 描画処理
	void Render();

	// GUI描画
	void DrawGUI();

	void Save();
	void Load();

	// ゲッター
	DirectX::XMFLOAT3 GetDefensePosition() const { return defensePosition; }
	float GetDefenseRadius() const { return defenseRadius; }

	// セッター
	void SetIsActive(bool flag) { isActive = flag; }
	bool GetIsActive() const { return isActive; }
private:
	DirectX::XMFLOAT3 defensePosition = { 0.0f, 1.0f, 0.0f };		  // 防御魔法の位置
	DirectX::XMFLOAT3 defensePositionOffset = { 0.0f, 0.0f, 0.0f }; // 防御魔法の位置のオフセット
	float defenseRadius = 2.0f; // 防御魔法の半径
	bool isActive = false; // 防御魔法を使っているかいないか
	Character* owner = nullptr;
};