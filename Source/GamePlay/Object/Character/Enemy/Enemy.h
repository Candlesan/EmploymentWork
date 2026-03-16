#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "System/UI/AnimationSequence.h"

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
	void InitializeAttackData();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox);

	// 計算したダメージ
	void SetLastDamage(float d) { lastDamage = d; }

	// プレイヤーとの距離を取得
	float GetDistanceToPlayer() const;

	// 移動速度セッター・ゲッター
	void SetMoveSpeed(float s) { moveSpeed = s; }
	float GetMoveSpeed() const { return moveSpeed; }

	// 旋回速度取得
	float GetTurnSpeed() const { return turnSpeed; }

	// 武器の当たり判定情報
	DirectX::XMFLOAT3 GetWeaponPosition(int index) const;
	DirectX::XMFLOAT3 GetWeaponDirection(int index) const;
	float GetWeaponRadius(int index) const { return weapon[index].weaponRadius; }
	float GetWeaponHeight(int index) const { return weapon[index].weaponHeight; }


	// 実行タイマー取得(仮実装)
	float GetRunTimer() { return runTimer; }
	// 実行タイマー設定(仮実装)
	void SetRunTimer(float timer) { runTimer = timer; }
	AnimationSequence<EnemyAnimationState>& GetAnimSequence() { return animSequence; }
private:
	std::shared_ptr<Model> GetModel() override { return enemy; }
	const std::shared_ptr<Model> GetModel() const override { return enemy; }

private:
	// 状態遷移更新処理
	void UpdateStateTransitions(float elapsedTime);

	// 武器のアタッチメント処理
	void WeaponAttachment();

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
		bool LeftHandInvincible = true;
		float weaponRadius = 0.5f;
		float weaponHeight = 1.0f;
	};
	Weapon weapon[2];


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

	// シーケンサー関係
	AnimationSequence<EnemyAnimationState> animSequence;
	int currentFrame = 0;
	bool sequencerExpanded = true;
	int selectedEntry = -1;
	int firstFrame = 0;

};