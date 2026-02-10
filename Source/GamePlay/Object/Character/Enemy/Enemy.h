#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "GamePlay/Object/Character/Character.h"
#include <memory>

class Enemy : public Character
{
public:
	Enemy() {};
	~Enemy() override {};

	void Initialize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer);

private:
	// アニメーション更新処理
	void UpdateAnimations(float elapsedTime);
private:
	std::shared_ptr<Model> enemy;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	enum class State
	{
		Idle = 0,
		Walk,
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

	DirectX::XMFLOAT4X4					worldTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

	// 当たり判定関係
	float debugOffset = 0.5;
};