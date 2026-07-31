#pragma once
// システム
#include "System/Renderer/ModelRenderer.h"
#include "System/UI/AnimationSequence/AnimationSequence.h"
#include "System/UI/AnimationSequence/AnimationSequencerEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"
#include "System/Audio/AudioSource.h"

// ゲームオブジェクト
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include "GamePlay/Object/Trail/Trail.h"
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"

#include <memory>

// 前方宣言
class Enemy;

class Player : public AnimationCharacter
{
public:
	// =========================================================================
	//  インスタンス管理（シングルトン）
	// =========================================================================
	static Player& Instance() {
		static Player instance;
		return instance;
	}

	// =========================================================================
	//  ライフサイクル（ゲームの根幹の流れ）
	// =========================================================================
	void Initialize();
	void InitializeSequencer();
	void InitializeAnimationNodeEditor();
	void InitializeHeadDirection();
	void SetupInitialAnimationState();
	void Finalize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void RenderTrail(const RenderContext& rc) { trail.Render(rc); }
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox = false);

	// =========================================================================
	//  外部からのアクション命令（プレイヤーを動かす外からのトリガー）
	// =========================================================================
	void MagicInput();
	void AddGraph(std::string name);              
	void UpdateSounds(const std::string& state);  
	void OnHitEnemyAttack(float damage, float poiseDamage, float invincibleTime);

	// =========================================================================
	//  セッター（外部からデータをセットする窓口）
	// =========================================================================
	void SetEnemy(Enemy* e) { enemy = e; }
	void SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos);
	//void SetLastDamage(float d) { lastDamage = d; }
	void SetIsHit(bool hit) { isHit = hit; }

	// =========================================================================
	//  ゲッター（外部からデータを取得する窓口）
	// =========================================================================
	// ── プレイヤーの状態 ──
	bool GetIsGuarding() const { return isGuarding; }
	bool GetIsAvoid() const { return isAvoid; }

	// ── 武器・戦闘関係 ──
	DirectX::XMFLOAT3 GetWeaponPosition() const;
	DirectX::XMFLOAT3 GetWeaponDirection() const;
	float GetWeaponRadius() const { return weapon.weaponRadius; }
	float GetWeaponHeight() const { return weapon.weaponHeight; }

	// ── マネージャー・システム系へのアクセス ──
	std::shared_ptr<Model> GetModel() override { return player; }
	const std::shared_ptr<Model> GetModel() const override { return player; }
	AnimationSequencer& GetAnimSequence() { return animSequence; }
	MagicManager& GetMagicManager() { return magicManager; }
	std::vector<HitBoxInfo> GetActiveHitBoxes() const override;
	float GetJustAvoidSlowTimer() const { return justAvoidSlowTimer; }

protected:
	// =========================================================================
	//  親子関係のオーバーライド（AnimationCharacterからの継承関数）
	// =========================================================================
	void OnLanding() override;
	void OnStateChanged(const std::string& oldState, const std::string& newState);

private:
	// コンストラクタ群（シングルトンなので隠す）
	Player() {};
	~Player() override {};

	// =========================================================================
	//  内部ヘルパー関数（UpdateやInputから小分けにされたお仕事）
	// =========================================================================
	// ── 移動・入力の計算 ──
	DirectX::XMFLOAT3 GetMoveVec() const;
	void InputMove(float elapsedTime);
	RelativeInputDir CalculateRelativeInputDir();

	// ── 専用技 ──
	void WindChargeAttack(float elapsedTime);

	// ── ステートマシン・先行入力 ──
	void UpdateStateTransitions(float elapsedTime);
	void UpdateUpperBodyTransitions(float elapsedTime);
	TransitionContext InputTransitionContext(float elapsedTime);
	void Player::EnterSubGraph(const std::string& subGraphPath, const TransitionContext& ctx);
	void PlayActionGraph(const std::string& graphName);
	void UpdateStateBehavior();

	// ── 物理・タイマー・その他 ──
	void UpdateTimers(float elapsedTime);
	void UpdateLockAt(float elapsedTime);
	void CalculateStatusParameters();
	void CalculationRootAndTip();
	void CalculateSpineRotation(float elapsedTime);
	AudioSource* GetOrLoadSound(const std::string& soundName);

	AnimNode* FindNodeByName(const std::string& stateName);

	void RefreshStateConfigs();
private:
	// =========================================================================
	//  メンバ変数（大量にあるので、ジャンルごとに綺麗にグループ化！）
	// =========================================================================
	// ── 3Dモデルと装備 ──
	std::shared_ptr<Model> player;
	Enemy* enemy = nullptr;

	// ── 基本パラメータ（移動・回転） ──
	float moveSpeed = 5.0f;
	float maxMoveSpeed = 7.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);
	float moveLength = 0.0f;

	// ── 戦闘・ステータス（HP・スタミナ・無敵） ──
	float invincibleTimer = 0.0f;
	float windDashAvailTimer = 0.3f;
	float runDisableTimer = 0.0f;
	float staminaCooldownTimer = 0.0f;
	bool haveStamina = false;
	bool isStaminaEmpty = false;

	// ── アクション状態のフラグ ──
	bool isGuarding = false;
	bool isHit = false;
	bool isAvoid = false;

	// ── ジャンプ関係 ──
	bool jumpPressed = false;
	float jumpSpeed = 7.0f;
	bool falling = false;
	int jumpCount = 0;
	int jumpLimit = 1;

	// ── ロックオン・敵との距離 ──
	const DirectX::XMFLOAT3* lockOnTargetPos = nullptr;
	float enemyFromDistance = 0.0f;
	float lockedEnemyDistance = 0.0f;

	// ── 魔法 ──
	MagicManager magicManager;
	NormalMagic::NormalMagicConfig normalMagicConfig;
	float magicCoolDownTimer = 0.0f;
	float coolDownValue = 0.0f;
	bool isMagicAttack = true;

	float DashSpeed = 20.0f;
	float magicRecoverDelay = 0.0f;

	// ── 特殊演出（背骨の傾き・武器の見た目・トレイル） ──
	float previousAngleY = 0.0f;
	float currentBankAngle = 0.0f;
	float targetSpineAngle = -10.0f;
	float spineAngleSpeed = 10.0f;
	Trail trail;
	DirectX::XMFLOAT3 rootOffset = { 0.42f, -5.93f, -0.37f };
	DirectX::XMFLOAT3 tipOffset = { 0.770f, 6.22f, -0.5f };
	DirectX::XMFLOAT3 swingAngle = { 0.0f, 0.0f, 1.6f };
	DirectX::XMFLOAT3 swingPosition = { 0.0f, 2.0f, 0.0f };
	DirectX::XMFLOAT3 swingOriginOffset = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 headLocalForward = { 0.0f, 0.0f, 1.0f };
	float rotationSpeed = 25.0f;
	float rotation = 0.0f;
	float dropSpeed = 5.0f;
	float dropStartT = 0.0f;
	float dropEndT = 0.0f;
	float justAvoidSlowTimer = 0.0f;
	float lookAtWeight = 0.0f;
	float WindDashAttackDistance = 0.0f;

	// ── 入力バッファ・閾値・デバッグ ──
	int bufferedButton = 0;
	float lerpSpeed = 3.0f;
	float bButtonHoldTime = 0.0f;
	float rbButtonHoldTime = 0.0f;
	float rtButtonHoldTime = 0.0f;
	float debug_degree = 0.0f;
	int debug_dirIndex = 0;
	float debugOffset = 0.5f;

	const float RB_THRESHOLD = 0.5f;
	float debugAngle = 0.0f;

	// ── シーケンサー・ノードエディタ ──
	AnimationSequencer animSequence;
	AnimationSequencerEditor sequencerEditor;
	AnimationTransitionEditor transitionEditor;
	std::vector<AnimationTransitionGraph> transitionGraphs;
	std::vector<int> activeGraphStack;
	std::vector<int> upperGraphStack;
	int currentGraphIndex = 0;
	int debugNextState = 0;

	// ── プレビューモード用 ──
	bool isPreviewMode = false;
	std::string editingState = "";

	// ── サウンド ──
	std::unordered_map<std::string, std::unique_ptr<AudioSource>> sounds;
};