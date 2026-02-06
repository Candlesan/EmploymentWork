#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "GamePlay/Object/Character/Character.h"
#include <memory>

class Player : public Character
{
public:
	Player() {};
	~Player() override {};

	void Initialize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();

private:
	// スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	// 入力処理
	void InputMove(float elapsedTime);
private:
	std::shared_ptr<Model> player;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);
};
