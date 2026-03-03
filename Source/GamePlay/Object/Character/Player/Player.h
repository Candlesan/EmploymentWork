#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "GamePlay/Object/Character/Character.h"
#include "GamePlay/Object/Character/Animation/AnimationCharacter.h"
#include <memory>

class Player : public AnimationCharacter<PlayerAnimationState>
{
public:
	Player() {};
	~Player() override {};

	void Initialize();
	void Update(float elapsedTime);
	void Render(RenderContext& rc, ModelRenderer* renderer);
	void DrawGUI();
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	// 武器の当たり判定情報
	DirectX::XMFLOAT3 GetWeaponPosition() const;
	DirectX::XMFLOAT3 GetWeaponDirection() const;
	float GetWeaponRadius() const { return weapon.weaponRadius; }
	float GetWeaponHeight() const { return weapon.weaponHeight; }

	// ロックオン対象の位置を取得
	void SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos); // nullptrかどうかでロックオンしてるか判定出来るらしい

protected:
	std::shared_ptr<Model> GetModel() override { return player; }
	const std::shared_ptr<Model> GetModel() const override { return player; }

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
};