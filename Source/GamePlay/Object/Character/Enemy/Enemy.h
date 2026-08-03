#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "System/UI/AnimationSequence/AnimationSequence.h"
#include "System/UI/AnimationSequence/AnimationSequencerEditor.h"
#include "System/UI/BehaviorTreeEditor/BehaviorTreeEditor.h"
#include "System/Audio/AudioSource.h"

#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorTree.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorData.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/NodeBase.h"

#include <memory>
#include <unordered_map>


class Enemy : public AnimationCharacter
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
	void TurnToPosition(float elapsedTime, const DirectX::XMFLOAT3& targetPos);
	void TurnToPosition(float elapsedTime);
	bool IsFacingTarget(const DirectX::XMFLOAT3& targetPos, float epsilonDegree);

	// ターゲットの座標に向かって進む
	void MoveToTarget(float elapsedTime, const DirectX::XMFLOAT3& targetPos);
	void MoveToTarget(float elapsedTime);

	// 移動速度セッター・ゲッター
	void SetMoveSpeed(float s) { moveSpeed = s; }
	float GetMoveSpeed() const { return moveSpeed; }

	// 体力
	void SetHealth(float h) { status.currentHp = h; }
	float GetHealth() { return status.currentHp; }
	void SetMaxHealth(float h) { status.maxHp = h; }
	float GetMaxHealth() { return status.maxHp; }

	// 旋回速度取得
	float GetTurnSpeed() const { return turnSpeed; }

	// 今使ってるHitBoxを計算して返す関数
	std::vector<HitBoxInfo> GetActiveHitBoxes() const;

	// 実行タイマー取得(仮実装)
	float GetRunTimer() { return runTimer; }
	// 実行タイマー設定(仮実装)
	void SetRunTimer(float timer) { runTimer = timer; }

	// 攻撃のクールダウン
	void SetAttackCoolTimer(float timer) { attackCoolTimer = timer; }
	float GetAttackCoolTimer() const { return attackCoolTimer; }

	// シーケンサーを取得
	AnimationSequencer& GetAnimSequence() { return animSequence; }

	void ClearActiveNode() { 
		activeNode = nullptr;
		lastInferredNode = nullptr;
	}
	NodeBase* GetActiveNode() const { return activeNode; }
	NodeBase* GetLastInferredNode() const { return lastInferredNode; }

	MagicManager& GetMagicManager()  { return magicManager; }

private:
	std::shared_ptr<Model> GetModel() override { return enemy; }
	const std::shared_ptr<Model> GetModel() const override { return enemy; }

private:
	// ダウン状態
	void OnDown() override;

	// アニメーションのコールバック関数
	void OnStateChanged(const std::string& oldState, const std::string& newState);

	// サウンドを流す
	void UpdateSounds(const std::string& state);

	// 音を取得（無ければ自動ロード）する関数
	AudioSource* GetOrLoadSound(const std::string& soundName);

	// GUIの表示/非表示
	void ShowDebugSwitch();
private:
	std::shared_ptr<Model> enemy;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	// アニメーション関係
	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	// 当たり判定関係
	float debugOffset = 0.5;

	// ダメージ関係
	float lastDamage = 0.0f;
	bool Ondown = false;
	float attackCoolTimer = 0.0f;

	// 距離
	float Short_Distance = 4.0f; // 近距離
	float Middle_Distance = 12.0; // 中距離
	float Long_Distance = 20.0; // 遠距離

	// ビヘイビアツリー関係
	BehaviorTree* aiTree = nullptr;
	BehaviorData* behaviorData = nullptr;
	NodeBase* activeNode = nullptr;
	NodeBase* lastInferredNode = nullptr;
	float runTimer = 0.0f;
	bool AISwitch = false;

	// SE関係
	// 音の名前と実体を紐づけるマップ
	std::unordered_map<std::string, std::unique_ptr<AudioSource>> sounds;

	// シーケンサー関係
	AnimationSequencer animSequence;
	AnimationSequencerEditor sequencerEditor;

	// --- プレビューモード用 ---
	bool isPreviewMode = false;
	std::string editingState = "";

	// ビヘイビアツリーノードエディター
	BehaviorTreeGraph btGraph;
	BehaviorTreeEditor btEditor;

	// 魔法関係
	MagicManager magicManager;

	//  --- デバック関係 ---
	bool showbtEditor = false;
	bool showEnemyParameter = true;
	bool showEnemy = true;
};