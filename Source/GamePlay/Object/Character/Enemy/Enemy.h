#pragma once

// システム
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "System/UI/AnimationSequence/AnimationSequence.h"
#include "System/UI/AnimationSequence/AnimationSequencerEditor.h"
#include "System/UI/BehaviorTreeEditor/BehaviorTreeEditor.h"
#include "System/Audio/AudioSource.h"

// ゲームオブジェクト
#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"

// ビヘイビアツリー
#include "GamePlay/Object/Character/Enemy/BehaviorTree/BehaviorTreeRunner.h"

// 魔法
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"
#include "GamePlay/Object/Magic/MagicType/RapidMagic/RapidMagic.h"

#include <memory>
#include <unordered_map>


class Enemy : public AnimationCharacter
{
public:
	// =========================================================================
	//  コンストラクタ・デストラクタ
	// =========================================================================
	Enemy() {};
	~Enemy();

	// =========================================================================
	//  ライフサイクル（ゲームの根幹の流れ）
	// =========================================================================
	void Initialize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	// =========================================================================
	//  外部アクション・AI制御
	// =========================================================================
	float GetCurrentMagic() const { return status.currentMagic; } // 現在の魔力量を返す

	// プレイヤーとの距離を取得
	float GetDistanceToPlayer() const;

	// ターゲットの座標に向かって旋回する
	void LockAtTarget(float elapsedTime, const DirectX::XMFLOAT3& targetPos);

	// ターゲットの方向を取得する(正規化済み)
	DirectX::XMFLOAT3 GetMoveDirection(const DirectX::XMFLOAT3& movePos) const;

	// =========================================================================
	//  セッター
	// =========================================================================
	// ── 基本パラメータ ──
	void SetMoveSpeed(float s) { moveSpeed = s; }

	// ── ステータス ──
	void SetHealth(float h) { status.currentHp = h; }
	void SetMaxHealth(float h) { status.maxHp = h; }

	// ── タイマー・AI制御 ──
	void SetRunTimer(float timer) { runTimer = timer; }
	void SetAttackCoolTimer(float timer) { attackCoolTimer = timer; }
	void SetAvoidCoolDownTimer(float timer) { avoidCoolDownTimer = timer; }
	void SetRandomValue(float value) { randomValue = value; }

	// =========================================================================
	//  ゲッター
	// =========================================================================
	// ── 基本パラメータ ──
	float GetMoveSpeed() const { return flightController.IsFlying() ? flightSpeed : moveSpeed; }
	float GetTurnSpeed() const { return turnSpeed; }

	// ── ステータス ──
	float GetHealth() { return status.currentHp; }
	float GetMaxHealth() { return status.maxHp; }

	// ── タイマー・戦闘 ──
	float GetRunTimer() const { return runTimer; } // 実行時間
	float GetAttackCoolTimer() const { return attackCoolTimer; } // 攻撃のクールダウン
	float GetIntercepRudius() const { return interceptRadius; } // 迎撃範囲の半径
	float GetAvoidCoolDownTimer() const{ return avoidCoolDownTimer; }
	float GetRandomValue() const { return randomValue; }

	// ── シーケンサー・マネージャー ──
	AnimationSequencer& GetAnimSequence() { return animSequence; }
	MagicManager& GetMagicManager() { return magicManager; }
	DefenseMagic& GetDefenseMagic() { return defenseMagic; }

	float GetCollisionOffset() const { return collisionOffset; }

protected:
	// =========================================================================
	//  親子関係のオーバーライド（AnimationCharacterからの継承関数）
	// =========================================================================
	std::shared_ptr<Model> GetModel() override { return enemy; }
	const std::shared_ptr<Model> GetModel() const override { return enemy; }

	void OnStateChanged(const std::string& oldState, const std::string& newState);

	void OnDead() override;
private:
	// =========================================================================
	//  内部ヘルパー関数
	// =========================================================================

	// ── サウンド ──
	void UpdateSounds(const std::string& state);
	AudioSource* GetOrLoadSound(const std::string& soundName);

	// ── ビヘイビアツリー ──
	void ClearActiveNode() { moveBT.Abort(); attackBT.Abort(); defenseBT.Abort(); }
	bool IsFullBodyAction();
	bool UpdateForcedState(float elapsedTime);

	// ── デバッグ ──
	void ShowDebugSwitch();
	void debugShot(float elapsedTime);

private:
	// =========================================================================
	//  メンバ変数（ジャンルごとにグループ化）
	// =========================================================================
	// ── 3Dモデル ──
	std::shared_ptr<Model> enemy;

	// ── 基本パラメータ（移動・回転） ──
	float moveSpeed = 5.0f;
	float flightSpeed = 20.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	// ── アニメーション関係 ──
	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	// ── 当たり判定・距離設定 ──
	float collisionOffset = 0.5;

	// ── ダメージ・戦闘ステータス ──
	float attackCoolTimer = 0.0f;
	float avoidCoolDownTimer = 0.0f;

	float randomValue = 0.0f;    // 0〜100 の乱数
	float randomTimer = 0.0f;    // 振り直しまでの時間
	float randomInterval = 1.0f; // 振り直す間隔（秒）

	enum class ForcedState { None, Down, GetUp, Burst, Break, Dead };
	ForcedState forcedState = ForcedState::None;
	bool wasBurstTrigger = false;
	bool wasBreak = false;

	// ── ビヘイビアツリー ──
	BehaviorTreeRunner moveBT; // 移動・回避・飛行用
	BehaviorTreeRunner attackBT; // 攻撃用
	BehaviorTreeRunner defenseBT; // 防御用
	float runTimer = 0.0f;
	bool AISwitch = false;

	// ── ビヘイビアツリーエディタ ──
	BehaviorTreeGraph moveGraph;
	BehaviorTreeEditor moveEditor;
	BehaviorTreeGraph attackGraph;
	BehaviorTreeEditor attackEditor;
	BehaviorTreeGraph defenseGraph;
	BehaviorTreeEditor defenseEditor;

	// ── シーケンサー・プレビュー ──
	AnimationSequencer animSequence;
	AnimationSequencerEditor sequencerEditor;
	bool isPreviewMode = false;
	std::string editingState = "";

	// ── 魔法関係 ──
	MagicManager magicManager;
	NormalMagic::NormalMagicConfig normalMagicConfig;
	NormalMagic normalMagic;
	RapidMagic::RapidMagicConfig rapidMagicConfig;
	RapidMagic rapidMagic;
	float magicCoolDownTimer = 0.0f;
	float coolDownValue = 1.0f;				// 通常攻撃魔法のクールダウン
	float fullcChargeCoolDownValue = 3.0f;	// フルチャージショット後のクールダウン
	bool rapidInterval = 0.1f;				// 連射魔法の発射間隔
	float magicRecoverDelay = 0.0f;			// 魔力を回復するまでの遅延
	float interceptRadius = 50.0f;			// 迎撃用の半径
	DirectX::XMFLOAT3 interceptCenterOffset = { 0.0f, 1.0f, 0.0f }; // 迎撃用のオフセット位置
	DirectX::XMFLOAT3 interceptDir = { 0.0f, 0.0f, 0.0f };

	// ── 飛行関係 ──
	float hoverHeightOffset = 5.0f;
	float hoverFollowRate = 1.0f;

	// ── デバッグ発射用フラグ ──
	bool onNormalMagic = false;
	bool onChargeMagic = false;
	bool onRapidMagic = false;

	// ── サウンド ──
	std::unordered_map<std::string, std::unique_ptr<AudioSource>> sounds;

	// ── デバッグGUI表示フラグ ──
	bool showbtEditor = true;
	bool showEnemyParameter = true;
	bool showEnemy = true;
};