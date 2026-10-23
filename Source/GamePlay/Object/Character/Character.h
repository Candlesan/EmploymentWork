#pragma once
// システム
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"

// ゲーム
#include "GamePlay/Object/Flight/FlightController.h"
#include "GamePlay/Object/LockOnSystem/LockOnSystem.h"

#include <DirectXMath.h>
#include <vector>

// 前方宣言
class Stage;

// キャラクターの基本情報
struct CharacterStatus
{
	// --- 体力系 ---
	float maxHp = 100.0f;				 // 最大体力
	float currentHp = 100.0f;			 // 現在の体力
										 
	// --- 魔力系 ---					  
	float maxMagic = 100.0f;			   // 最大魔力
	float currentMagic = 100.0f;		   // 現在の魔力量
	float magicRecoverSpeed = 5.0f;        // 基本の回復速度
	float guardMagicRecoverTimer = 0.0f;   // 1秒計測用のタイマー
	float guardMagicRecoverAmount = 3.0f;  // 1秒ごとに回復する量
	float currentMagicRecoverSpeed = 5.0f; // 自然回復速度
	float magicRecoverDelayTimer = 0.0f;   // 魔力が回復し始めるまでの遅延

	float interceptMagicCost = 10.0f;	// 迎撃中の魔力消費量

	// --- 防御系 ---
	float maxDefenseDurability = 100.0f;		// 防御魔法の最大耐久値
	float currentDefenseDurability = 100.0f;	// 現在の防御魔法の耐久値
	float defenseRecoverSpeed = 5.0f;			// 防御魔法の耐久値の回復速度
	float defenseMagicCost = 3.0f;				// 防御魔法の魔力消費量

	// --- 状態フラグ ---
	bool isDown = false;			// 魔力切れダウン中か
	bool isBreak = false;			// 防御魔法が割れたか
	bool isRapidFire = false;		// 連射攻撃か
	bool isIntercept = false;		// 迎撃中か
	bool isGuarding = false;		// ガード中か
	bool isFlying = false;			// 飛行中か
	bool isHovering = false;		// 滞空中か
	bool isAvoid = false;			// 回避したか
	bool isDead = false;			// 死亡したか
};

class Character
{
public:
	Character() {}
	virtual ~Character() = default;

	// ----- 基本操作 -----

	// 行列（transform）更新
	void UpdateTransform();

	// 位置取得 / 設定
	const DirectX::XMFLOAT3& GetPosition() const { return position; }
	void SetPosition(const DirectX::XMFLOAT3& position) { this->position = position; }

	// 回転取得 / 設定
	const DirectX::XMFLOAT3& GetAngle() const { return angle; }
	void SetAngle(const DirectX::XMFLOAT3& angle) { this->angle = angle; }

	// スケール取得 / 設定
	const DirectX::XMFLOAT3& GetScale() const { return scale; }
	void SetScale(const DirectX::XMFLOAT3& scale) { this->scale = scale; }

	// 行列取得 / 設定
	const DirectX::XMFLOAT4X4& GetTransform() const { return transform; }
	void Set_Transform(DirectX::XMFLOAT4X4 new_transform) { this->transform = new_transform; }

	// モデル取得
	virtual std::shared_ptr<Model> GetModel() { return nullptr; }

	// 速度取得 / 設定
	const DirectX::XMFLOAT3& GetVelocity() const { return velocity; }
	void SetVelocity(const DirectX::XMFLOAT3& v) { velocity = v; }

	// 当たり判定情報取得
	float GetRadius() const { return radius; }
	float GetHeight() const { return height; }
	float GetWeight() const { return weight; }

	// 地面状態
	bool IsGround() const { return isGround; }
	bool SetIsGround(bool flag) { return isGround = flag; }

	void SetGravity(float g) { gravity = g; }

	// ----- ダメージ / ステータス -----

	// ダメージを与える（無敵時間・体幹ダメージを考慮）
	virtual bool ApplyDamage(float damage);

	// ステータス更新（毎フレーム呼ぶ）
	void UpdateStatus(float elapsedTime);

	// 魔力消費
	void ConsumMagic(float amount, float DelayTime);
	void ConsumMagic(float amount);

	// 魔力回復
	void RecoverMagic(float amount);

	// ガード耐久値減少
	void OnGuardHit(float amount);

	void SetIsRapidFire(bool flag) { status.isRapidFire = flag; }
	void SetIsIntercept(bool flag) { status.isIntercept = flag; }

	// 防御魔法を展開しているか
	bool GetIsGaurd() const { return status.isGuarding; }
	void SetIsGaurd(bool flag) { status.isGuarding = flag; }
	// ----- 移動 / 回転 -----

	// 移動処理（入力 vx, vz, speed を受け取る）
	void Move(float vx, float vz, float speed);

	// 旋回処理（経過時間と入力）
	void Turn(float elapsedTime, float vx, float vz, float speed);

	// カプセルの方向取得（上方向）
	DirectX::XMFLOAT3 GetCapsuleDirection() const
	{
		return DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
	}

	// ステータスを取得する
	const CharacterStatus& GetStatus() const { return status; }
	void SetStatus(CharacterStatus& s) { status = s; }

	// 自動照準システムを取得
	LockOnSystem&  GetLockOnSystem() { return lockOn; }

	// 回避中か
	bool GetIsAvoid() const { return status.isAvoid; }
	void SetIsAvoid(bool flag) { status.isAvoid = flag; }

	// 飛行を取得
	FlightController& GetFlightController() { return flightController; }
	const FlightController& GetFlightController() const { return flightController; }

	// シーンゲームからもらってくるステージ
	void SetStage(const Stage* stage) { this->stage = stage; }

	// 今死んだか
	bool IsDead() const { return status.isDead; }
private:
	// ----- 内部：移動・速度更新ヘルパー -----

	// 水平速力更新処理
	void UpdateHorizontalVelocity(float elapsedTime);

	// 水平移動更新処理
	void UpdateHorizontalMove(float elapsedTime);

	// 垂直速力更新処理
	void UpdateVerticalVelocity(float elapsedTime);

	// 垂直移動更新処理
	void UpdateVerticalMove(float elapsedTime);

	bool MoveAndSlide(const DirectX::XMFLOAT3& move, bool vertical);

protected:
	// ジャンプ処理（派生で呼ぶ）
	void Jump(float speed);

	// 速度処理の総合更新
	void UpdateVelocity(float elapsedTime);

	// ----- 継承先フック -----

	// 着地したときに呼ばれる
	virtual void OnLanding() {}

	// ダメージを受けた時に呼ばれる
	virtual void OnDamage() {}

	// 死亡した時に呼ばれる
	virtual void OnDead() {}

	// ダウンした時に呼ばれる
	virtual void OnDown() {}

protected:
	CharacterStatus status;		  // 現在のステータス
	CharacterStatus defaltStatus; // 初期データ
	FlightController flightController; // 飛行情報
	LockOnSystem lockOn;

	// 位置・回転・スケール・行列
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 angle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT4X4 transform =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// 速度
	DirectX::XMFLOAT3 velocity = { 0.0f, 0.0f, 0.0f };

	// 移動関連
	float maxMoveSpeed = 20.0f;   // 最大速度
	float moveVecX = 0.0f;   // 移動ベクトル（X軸方向）
	float moveVecZ = 0.0f;   // 移動ベクトル（Z軸方向）

	float friction = 15.0f;  // 摩擦力
	float acceleration = 50.0f;  // 加速度
	float gravity = -9.8f;  // 重力

	// 当たり判定
	float radius = 0.5f;  // 半径
	float height = 2.0f;  // 高さ
	float weight = 0.5f;  // 重さ

	bool isGround = true; // 地面と接地しているか

	// 接地関係
	const Stage* stage = nullptr;
	float stepOffset = 0.3f; // 足元からの補正
	float slopeLimit = DirectX::XMConvertToRadians(45.0f); // 角度で滑らせるか
	float collisionRadius = 0.8f;
	float BurstCoolDownScale = 3.0f; // バースト時のヒート値の回復倍率
};