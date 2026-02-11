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
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	// 武器の当たり判定情報
	DirectX::XMFLOAT3 GetWeaponPosition() const;
	DirectX::XMFLOAT3 GetWeaponDirection() const;
	float GetWeaponRadius() const { return weapon.weaponRadius; }
	float GetWeaponHeight() const { return weapon.weaponHeight; }
private:
	// スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	// 入力処理
	void InputMove(float elapsedTime);

	// アニメーション更新処理
	void UpdateAnimations(float elapsedTime);

	// アニメーションが終了したか
	bool IsFinshedAnimation();

	// 武器のアタッチメント処理
	void WeaponAttachment();

private:
	std::shared_ptr<Model> player;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	// アニメーション関係
	enum class State
	{
		Idle = 0,
		Walk,
		Attack,
	};
	State state = State::Idle;

	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	int animationIndex = -1;
	float animationSeconds = 0.0f;
	float oldAnimationSeconds = 0.0f;
	float animationBlendSeconds = 0.2f;
	float animationBlendSecondsLength = 0.2f;
	bool isBlending = true;
	bool animationLoop = true;

	bool useRootMotion = false;
	bool useRootMotionEx = false;
	bool bakeTranslationY = true; // Y軸移動を無視するか
	DirectX::XMFLOAT3 rootMotionPosition = { 0, 0, 0 }; // ルートモーションによる位置

	DirectX::XMFLOAT4X4	worldTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	// 武器のアタッチメント関係
	struct Weapon
	{
		std::shared_ptr<Model> model;
		DirectX::XMFLOAT3 position = { 0,0,0 };
		DirectX::XMFLOAT3 angle = { 0,0,0 };
		DirectX::XMFLOAT3 scale = { 1,1,1 };
		DirectX::XMFLOAT4X4 transform = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
		DirectX::XMFLOAT3 weaponHitOffset = { 0, 0, 0 };
		DirectX::XMFLOAT3 weaponAngleOffset = { 0, 0, 0 };
		float weaponRadius = 0.5f;
		float weaponHeight = 1.0f;
	};
	Weapon weapon;

	// 当たり判定関係
	float debugOffset = 0.5;
};