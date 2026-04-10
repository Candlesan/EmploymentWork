#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "System/UI/AnimationSequence.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionEditor.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"

#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include <memory>

class Player : public AnimationCharacter<PlayerAnimationState>
{
private:
	Player() {};
	~Player() override {};

public:
	// 唯一のインスタンス
	static Player& Instance()
	{
		static Player instance;
		return instance;
	}

	void Initialize();
	void InitializeAttackData();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox = false);

	// 武器の当たり判定情報
	DirectX::XMFLOAT3 GetWeaponPosition() const;
	DirectX::XMFLOAT3 GetWeaponDirection() const;
	float GetWeaponRadius() const { return weapon.weaponRadius; }
	float GetWeaponHeight() const { return weapon.weaponHeight; }

	// 回復
	void Heal(float elapsedTime);

	// スタミナ計算処理
	void calculationStamina(float stamina);

	// スタミナ回復処理
	void RecoveryStamina(float elapsedTime);

	// ロックオン対象の位置を取得
	void SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos); // nullptrかどうかでロックオンしてるか判定出来るらしい

	std::shared_ptr<Model> GetModel() override { return player; }
	const std::shared_ptr<Model> GetModel() const override { return player; }
	AnimationSequence<PlayerAnimationState>& GetAnimSequence() { return animSequence; }
	void SetLastDamage(float d) { lastDamage = d; }	// 計算したダメージ
	bool GetIsGuarding() const { return IsGuarding; } // 防御中かどうか
	void SetIsHit(bool hit) { IsHit = hit; } // 攻撃を食らったかどうか設定する
	bool GetIsAvoid() const { return IsAvoid; } // 回避中かを取得

public:
	// グラフを追加する
	void AddGraph(std::string name);

protected:

	//着地したときに呼ばれる
	void OnLanding() override;
private:
	// スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec() const;

	// 入力処理
	void InputMove(float elapsedTime);

	// ジャンプ出来るかどうか
	bool CanJump() const;

	// 状態遷移更新処理
	void UpdateStateTransitions(float elapsedTime);

	// 遷移以外の細かい条件
	void UpdateStateBehavior();

	// 歩きのアニメションを決める関数
	PlayerAnimationState DetermineWalkState(); 

	// 回避のアニメションを決める関数
	PlayerAnimationState DetermineRollState(); 

	// 武器のアタッチメント処理
	void WeaponAttachment();
private:
	std::shared_ptr<Model> player;

	float moveSpeed = 5.0f;
	float turnSpeed = DirectX::XMConvertToRadians(720);

	// 武器のアタッチメqaント関係
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
	float lastDamage = 0.0f;

	enum class MoveMode {
		Walk,   // 通常時
		Jog,	// 小走り
		Run,	// 走り
		Guarding_Walk, // ガード入力中(歩き)
		Guarding_Jog,  // ガード入力中(小走り)
	};
	MoveMode mode = MoveMode::Walk;

	// 当たり判定関係
	float debugOffset = 0.5;

	// 回避用の条件
	float bButtonHoldTime = 0.0f;          // Bボタンを押し続けた時間
	static constexpr float RUN_THRESHOLD = 0.5f; // 何秒以上で走りと判定するか

	// 攻撃用の条件
	float rtButtonHoldTime = 0.0f;          // RTボタンを押し続けた時間
	static constexpr float ATTACK_THRESHOLD = 0.25f; // 何秒以上で溜め攻撃と判定するか

	bool IsGuarding = false; // ガード中か
	bool IsHit = false; // 攻撃を食らったか
	bool IsAvoid = false; // 回避が有効か

	// ジャンプ関係
	bool jumpPressed = false;
	float jumpSpeed = 5.0f;

	int jumpCount = 0;
	int jumpLimit = 1;

	// ロックオン関係
	const DirectX::XMFLOAT3* lockOnTargetPos = nullptr;
	float lerpSpeed = 10.0f;
	float debug_degree = 0.0f;   // 入力角度
	int   debug_dirIndex = 0;    // 方向インデックス

	// シーケンサー関係
	AnimationSequence<PlayerAnimationState> animSequence;
	int currentFrame = 0;
	bool sequencerExpanded = true;
	int selectedEntry = -1;
	int firstFrame = 0;

	// 回復関係
	int Potion = 14; // ポーション数
	float HealValue = 810; // 回復量
	float HealCoolDownTimer = 0.0f; // クールダウン
	float RegeneTimer = 0.0f; // 回復中

	// スタミナ関係
	float staminaCooldownTimer = 0.0f;
	bool HaveStamina = false;
	bool IsStaminaEmpty = false;
	float runDisableTimer = 0.0f;

	// ノードエディター
	std::vector<AnimationTransitionGraph>   transitionGraphs;
	AnimationTransitionEditor  transitionEditor;
	int currentGraphIndex = 0;
	int debugNextState = 0;
};