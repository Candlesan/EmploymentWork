#pragma once
// システム
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "System/UI/AnimationSequence/AnimationSequence.h"
#include "System/UI/AnimationSequence/AnimationSequencerEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffect.h"
#include "System/Audio/AudioSource.h"

// ゲームオブジェクト
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include "GamePlay/Object/Trail/Trail.h"
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"
#include "GamePlay/Object/Magic/MagicType/RapidMagic/RapidMagic.h"
#include "GamePlay/Object/Magic/MagicType/DefenseMagic/DefenseMagic.h"

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
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	// =========================================================================
	//  外部からのアクション命令（プレイヤーを動かす外からのトリガー）
	// =========================================================================
	void MagicInput(float elapsedTime);
	void AddGraph(std::string name);
	void UpdateSounds(const std::string& state);
	void InputLockOn();

	// =========================================================================
	//  セッター
	// =========================================================================
	void SetEnemy(Enemy* e) { enemy = e; }
	void SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos);
	void ClearLockOnTarget();

	// =========================================================================
	//  ゲッター
	// =========================================================================
	// ── プレイヤーの状態 ──
	float GetCurrentMagic() const { return status.currentMagic; } // 現在の魔力量を返す
	float GetCurrentDefense() const { return status.currentDefenseDurability; } // 現在の防御魔法の耐久値を返す

	LockOnSystem& GetLockOnSystem() { return lockOn; }
	const LockOnSystem& GetLockOnSystem() const { return lockOn; }

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
	DefenseMagic GetDefenseMagic() { return defenseMagic; }

	float GetCollisonOffset() const { return CollisionOffset; }
protected:
	// =========================================================================
	//  親子関係のオーバーライド（AnimationCharacterからの継承関数）
	// =========================================================================

	// 死んだときに呼ばれる奴
	void OnDead() override;
private:
	// コンストラクタ群（シングルトンなので隠す）
	Player() {};
	~Player() override {};

	// =========================================================================
	//  内部ヘルパー関数
	// =========================================================================
	// ── 移動・入力の計算 ──
	DirectX::XMFLOAT3 GetMoveVec() const;
	void InputMove(float elapsedTime);
	RelativeInputDir CalculateRelativeInputDir();


	// ── ステートマシン・先行入力 ──
	void UpdateStateTransitions(float elapsedTime);
	void UpdateUpperBodyTransitions(float elapsedTime);
	TransitionContext InputTransitionContext(float elapsedTime);
	void Player::EnterSubGraph(const std::string& subGraphPath, const TransitionContext& ctx);
	void PlayActionGraph(const std::string& graphName);
	void UpdateStateBehavior();
	bool IsDownState();
	bool IsReviveState();
	bool CanControl();

	// ── 物理・タイマー・その他 ──
	void UpdateTimers(float elapsedTime);
	void UpdateLockAt(float elapsedTime);
	void CalculateStatusParameters();
	//void CalculationRootAndTip();
	void CalculateSpineRotation(float elapsedTime);

	// 音を取得（無ければ自動ロード）する関数
	AudioSource* GetOrLoadSound(const std::string& soundName);

	//　ノードの名前を探す関数
	AnimNode* FindNodeByName(const std::string& stateName);

	// ノードの設定を最新にする関数
	void RefreshStateConfigs();

	// GUIの表示/非表示
	void ShowDebugSwitch();
private:
	// =========================================================================
	//  メンバ変数（大量にあるので、ジャンルごとに綺麗にグループ化！）
	// =========================================================================
	// ── 3Dモデルと装備 ──
	std::shared_ptr<Model> player;
	Enemy* enemy = nullptr;

	// ── 基本パラメータ（移動・回転） ──
	float moveSpeed = 5.0f;
	float fullChargeSpeed = 15.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);
	float moveLength = 0.0f;

	// ── ロックオン・敵との距離 ──
	const DirectX::XMFLOAT3* lockOnTargetPos = nullptr;
	bool hasLockOnTarget = false;
	float enemyFromDistance = 0.0f;
	float lockedEnemyDistance = 0.0f;

	// ── 魔法 ──
	MagicManager magicManager;
	NormalMagic::NormalMagicConfig normalMagicConfig;
	NormalMagic normalMagic;
	RapidMagic::RapidMagicConfig rapidMagicConfig;
	RapidMagic rapidMagic;
	DefenseMagic defenseMagic;
	float magicCoolDownTimer = 0.0f;
	float coolDownValue = 1.0f;				// 通常攻撃魔法のクールダウン
	float fullcChargeCoolDownValue = 3.0f;	// フルチャージショット後のクールダウン
	bool rapidInterval = 0.1f;				// 連射魔法の発射感覚
	float magicRecoverDelay = 0.0f;			// 魔力を回復するまでの遅延
	float interceptRadius = 50.0f;			// 迎撃用の半径
	DirectX::XMFLOAT3 interceptCenterOffset = { 0.0f, 1.0f, 0.0f }; // 迎撃用のオフセット位置
	DirectX::XMFLOAT3 interceptDir = { 0.0f, 0.0f, 0.0f };

	// ── 特殊演出（背骨の傾き・武器の見た目・トレイル） ──
	float previousAngleY = 0.0f;
	float currentBankAngle = 0.0f;
	float targetSpineAngle = -10.0f;
	float spineAngleSpeed = 10.0f;
	DirectX::XMFLOAT3 rootOffset = { 0.42f, -5.93f, -0.37f };
	DirectX::XMFLOAT3 tipOffset = { 0.770f, 6.22f, -0.5f };
	DirectX::XMFLOAT3 swingAngle = { 0.0f, 0.0f, 1.6f };
	DirectX::XMFLOAT3 swingPosition = { 0.0f, 2.0f, 0.0f };
	DirectX::XMFLOAT3 swingOriginOffset = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 headLocalForward = { 0.0f, 0.0f, 1.0f };
	float rotationSpeed = 25.0f;
	float rotation = 0.0f;
	float dropStartT = 0.0f;
	float dropEndT = 0.0f;
	float lookAtWeight = 0.0f;

	// ── 入力バッファ・閾値・デバッグ ──
	int bufferedButton = 0;
	float lerpSpeed = 3.0f;
	float bButtonHoldTime = 0.0f;
	float rbButtonHoldTime = 0.0f;
	float rtButtonHoldTime = 0.0f;
	float debug_degree = 0.0f;
	int debug_dirIndex = 0;
	float CollisionOffset = 0.5f;

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

	// エフェクト関係
	EffectHandle barrierEffect = InvalidEffect; // 防御魔法のエフェクト

	// ── デバック ──
	bool showAnimarionTransitionEditor = true;
	bool showPlayerParameter = true;
	bool showPlayer = true;
	bool wasBreak = false;
};