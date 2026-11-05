#pragma once
// システム
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "System/UI/AnimationSequence/AnimationSequence.h"
#include "System/UI/AnimationSequence/AnimationSequencerEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"
#include "System/Audio/AudioSource.h"

// ゲームオブジェクト
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicType/NormalMagic/NormalMagic.h"
#include "GamePlay/Object/Magic/Loadout/MagicLoadout.h"
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"

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
	void UpdateFly(float elapsedTime);

	// =========================================================================
	//  セッター
	// =========================================================================
	void SetEnemy(Enemy* e) { enemy = e; }

	// =========================================================================
	//  ゲッター
	// =========================================================================
	// ── プレイヤーの状態 ──
	float GetCurrentMagic() const { return status.currentMagic; } // 現在の魔力量を返す

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
	DefenseMagic& GetDefenseMagic() { return defenseMagic; }

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
	bool IsDownState();
	bool IsReviveState();
	bool CanControl();

	// ── 物理・タイマー・その他 ──
	void UpdateTimers(float elapsedTime);
	void CalculateStatusParameters();
	void CalculateSpineRotation(float elapsedTime);

	// 音を取得（無ければ自動ロード）する関数
	AudioSource* GetOrLoadSound(const std::string& soundName);

	//　ノードの名前を探す関数
	AnimNode* FindNodeByName(const std::string& stateName);

	// ノードの設定を最新にする関数
	void RefreshStateConfigs();

	// GUIの表示/非表示
	void ShowDebugSwitch();

	void TurnToPosition(float elapsedTime, const DirectX::XMFLOAT3& target, float speed);

	// ルックアット処理
	void UpdateLockAt(float elapsedTime);

	// 飛行中に身体を傾ける処理
	void bank(float elapsedTime);

	// 腕のIK
	void ArmIK(float elapsedTime);

	// カメラの照準の先で、最初に当たる点を返す
	DirectX::XMFLOAT3 GetAimPoint(float range); 

	// 魔法を作る関数
	MagicContext MakeMagicContext();

	// 敵の方を向き続けているか（向きと回避の両方で使う）
	bool IsFacingEnemy() const;
private:
	// =========================================================================
	//  メンバ変数
	// =========================================================================
	// ── 3Dモデルと装備 ──
	std::shared_ptr<Model> player;
	Enemy* enemy = nullptr;

	// ── 基本パラメータ（移動・回転） ──
	float moveSpeed = 5.0f;
	float flightSpeed = 20.0f;
	float fullChargeSpeed = 15.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);
	float moveLength = 0.0f;
	bool falling = false;

	// ── ロックオン・敵との距離 ──
	bool hasLockOnTarget = false;
	bool hardLock = false;
	float enemyFromDistance = 0.0f;
	float lockedEnemyDistance = 0.0f;

	// ── 魔法 ──
	MagicManager magicManager;
	NormalMagic::NormalMagicConfig normalMagicConfig;
	NormalMagic normalMagic;

	MagicInputController magicInputController;
	MagicLoadout magicLoadout;

	float magicCoolDownTimer = 0.0f;
	float coolDownValue = 1.0f;				// 通常攻撃魔法のクールダウン
	float fullcChargeCoolDownValue = 3.0f;	// フルチャージショット後のクールダウン
	float magicRecoverDelay = 0.0f;			// 魔力を回復するまでの遅延
	DirectX::XMFLOAT3 muzzlePos = { 0, 0, 0 }; // 発射方向
	DirectX::XMFLOAT3 defultTarget = { 0, 0, 0 }; // 初期の狙ってる位置
	bool isShot = false;

	// ── IK関係 ──
	DirectX::XMFLOAT3 headLocalForward = { 0.0f, 0.0f, 1.0f }; // 頭の向きを保存
	float previousAngleY = 0.0f; // 前フレームの背骨の傾きを保存
	float currentBankAngle = 0.0f; // 現在の背骨の傾き
	float targetSpineAngle = -10.0f; // 目標の腰骨の角度
	float spineAngleSpeed = 10.0f; // 腰骨の回転速度
	float lookAtWeight = 0.0f; // ルックアットの重み
	float flightBankX = 0.0f; // 飛行中のX軸の傾き
	float flightBankZ = 0.0f; // 飛行中のZ軸の傾き
	float bankSpeed = 5.0f; // 傾く速度
	float armAimWeight = 0.0f;           // 腕を向ける強さ（0～1）
	float interceptAimHeight = 1.2f;     // 敵のどの高さを狙うか
	float interceptArmMaxAngle = 90.0f;  // 腕を回せる最大角度

	// ── 入力バッファ・閾値・デバッグ ──
	int bufferedButton = 0;
	bool isSubGraphExited = false; // 子グラフからExitした直後か（親ではアニメ終了扱いにする）
	float lerpSpeed = 3.0f;
	float bButtonHoldTime = 0.0f;
	float rbButtonHoldTime = 0.0f;
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
	bool isTyping = false;

	float interceptDistanceStopSeconds = 0.348f;
};