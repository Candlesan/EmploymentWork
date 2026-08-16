#pragma once
#include <DirectXMath.h>
#include <vector>
#include "System/Renderer/ModelRenderer.h"
#include "System/Renderer/ShapeRenderer.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/Flight/FlightController.h"


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

	float rapidFireMagicCost = 6.0f;	// 連射魔法の魔力消費量

	// --- ヒート値 ---
	float maxHeat = 100.0f;				// 最大ヒート値
	float currentHeat = 0.0f;			// 現在のヒート値
	float heatIncreaseSpeed = 20.0f;		// ヒート値の増加スピード
	float heatRecoverSpeed = 5.0f;		// ヒート値の回復速度

	// --- 防御系 ---
	float maxDefenseDurability = 100.0f;		// 防御魔法の最大耐久値
	float currentDefenseDurability = 100.0f;	// 現在の防御魔法の耐久値
	float defenseRecoverSpeed = 5.0f;			// 防御魔法の耐久値の回復速度
	float defenseMagicCost = 3.0f;				// 防御魔法の魔力消費量

	// --- ダウン系 ---
	float downTimer = 0.0f; // 魔力切れをしたときに使うタイマー

	// --- 状態フラグ ---
	bool isDown = false;		// 魔力切れダウン中か
	bool isBurst = false;		// オーバーヒートしたか
	bool isRapidFire = false;	// 連射攻撃か
	bool isGuarding = false;	// ガード中か
	bool isFlying = false;		// 飛行中か
	bool isIntercept = false;   // 迎撃中か
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

	// ステージ外移動制限
	void AreaRestriction();

	// ステータスを取得する
	const CharacterStatus& GetStatus() const { return status; }
	void SetStatus(CharacterStatus& s) { status = s; }

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
	float maxMoveSpeed = 5.0f;   // 最大速度
	float moveVecX = 0.0f;   // 移動ベクトル（X軸方向）
	float moveVecZ = 0.0f;   // 移動ベクトル（Z軸方向）

	float friction = 15.0f;  // 摩擦力
	float acceleration = 50.0f;  // 加速度
	float gravity = -9.8f;  // 重力

	// 当たり判定
	float radius = 0.5f;  // 半径
	float height = 2.0f;  // 高さ
	float weight = 0.5f;  // 重さ

	bool isGround = false; // 地面と接地しているか

	// ステージの広さ
	float stageRadius = 97.5f;
};