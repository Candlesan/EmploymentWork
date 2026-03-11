#pragma once
#include "System/Renderer/ModelRenderer.h"

#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorTree.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorData.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/NodeBase.h"

#include <memory>

class Enemy : public AnimationCharacter<EnemyAnimationState>
{
public:
	Enemy() {};
	~Enemy() override {};

	void Initialize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	// 計算したダメージ
	void SetLastDamage(float d) { lastDamage = d; }

	// プレイヤーとの距離を取得
	float GetDistanceToPlayer() const;

	// 移動速度セッター・ゲッター
	void SetMoveSpeed(float s) { moveSpeed = s; }
	float GetMoveSpeed() const { return moveSpeed; }

	// 旋回速度取得
	float GetTurnSpeed() const { return turnSpeed; }

	// 実行タイマー取得(仮実装)
	float GetRunTimer() { return runTimer; }
	// 実行タイマー設定(仮実装)
	void SetRunTimer(float timer) { runTimer = timer; }
private:
	std::shared_ptr<Model> GetModel() override { return enemy; }
	const std::shared_ptr<Model> GetModel() const override { return enemy; }

private:
	// 状態遷移更新処理
	void UpdateStateTransitions(float elapsedTime);

	// ダウン状態
	void OnDown() override;
private:
	std::shared_ptr<Model> enemy;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	// アニメーション関係
	enum class State
	{
		Idle = 0,
		Walk,
		Down,
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

	// ダメージ関係
	float lastDamage = 0.0f;
	bool Ondown = false;

	// 距離
	float distance = 0.0f;

	// ビヘイビアツリー関係
	BehaviorTree* aiTree = nullptr;
	BehaviorData* behaviorData = nullptr;
	NodeBase* activeNode = nullptr;
	float runTimer = 0.0f;
};