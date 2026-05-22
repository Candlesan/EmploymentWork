#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "System/UI/AnimationSequence.h"
#include "System/Audio/AudioSource.h"

#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorTree.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorData.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/NodeBase.h"

#include <memory>
#include <unordered_map>


class Enemy : public AnimationCharacter<EnemyAnimationState>
{
public:
	Enemy() {};
	~Enemy();

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

	// ターゲットの座標に向かって旋回する
	void Enemy::TurnToPosition(float elapsedTime, const DirectX::XMFLOAT3& targetPos);
	void Enemy::TurnToPosition(float elapsedTime);
	bool Enemy::IsFacingTarget(const DirectX::XMFLOAT3& targetPos, float epsilonDegree);

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

	// アクティブな球判定をすべて取得する
	struct SphereHitInfo {
		DirectX::XMFLOAT3 position;
		float radius;
	};
	std::vector<SphereHitInfo> GetActiveSphereHits() const;

	// 始動技を決める関数
	EnemyAnimationState Enemy::DecideFirstAttack();

	// 技の派生があるか確認する関数
	EnemyAnimationState Enemy::DecideNextAttack(EnemyAnimationState currentState);

	// スタンプ攻撃
	void HeavyStompAttack();

	// 実行タイマー取得(仮実装)
	float GetRunTimer() { return runTimer; }
	// 実行タイマー設定(仮実装)
	void SetRunTimer(float timer) { runTimer = timer; }

	// 攻撃のクールダウン
	void SetAttackCoolTimer(float timer) { attackCoolTimer = timer; }
	float GetAttackCoolTimer() const { return attackCoolTimer; }

	// シーケンサーを取得
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

	// アニメーションのコールバック関数
	void OnStateChanged(EnemyAnimationState oldState, EnemyAnimationState newState);

	// シーケンサーを描画する
	void EnemyAnimationSequencer();

	// サウンドを流す
	void UpdateSounds(EnemyAnimationState state);

	// 音を取得（無ければ自動ロード）する関数
	AudioSource* GetOrLoadSound(const std::string& soundName);
private:
	std::shared_ptr<Model> enemy;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	// アニメーション関係

	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	// 武器のアタッチメント関係
	struct Weapon
	{
		std::shared_ptr<Model> model;
		DirectX::XMFLOAT3 position = { 0,0,0 };
		DirectX::XMFLOAT3 angle = { 0,0,0 };
		DirectX::XMFLOAT3 scale = { 1.5,1,1 };
		DirectX::XMFLOAT4X4 transform = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
		DirectX::XMFLOAT3 weaponHitOffset = { 0, 0, 0 };
		DirectX::XMFLOAT3 weaponAngleOffset = { 0, 0, 0 };
		bool RightHandInvincible = false;
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
	float Short_Distance = 4.0f; // 近距離
	float Middle_Distance = 12.0; // 中距離
	float Long_Distance = 20.0; // 遠距離

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

	// 技のつながり
	struct AttackDerivation 
	{
		EnemyAnimationState nextState; // 次に出す技
		float minDistance; // 派生するための最低距離
		float maxDistance; // 派生するための最大距離
		int probability; // 派生する確率（0～100）
	};
	std::unordered_map<EnemyAnimationState, std::vector<AttackDerivation>> attackComboMap;

	// 始動技
	struct FirstAttack
	{
		EnemyAnimationState state;
		float minRange;
		float maxRange;
	};
	std::vector<FirstAttack> firstAttackList;

	float attackCoolTimer = 0.0f;

	// SE関係
// 音の名前と実体を紐づけるマップ
	std::unordered_map<std::string, std::unique_ptr<AudioSource>> sounds;
};