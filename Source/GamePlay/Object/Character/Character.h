#pragma once
#include <DirectXMath.h>
#include <vector>
#include "System/Renderer/ModelRenderer.h"
#include "System/Renderer/ShapeRenderer.h"
#include "GamePlay/Object/Collision/Collision.h"


// ダメージの種類（返り値用の構造体）
struct AttackResult
{
	float damage;
	float poiseDamage;
};

class Character
{
public:
	// 当たり判定情報
	struct HitBoxInfo
	{
		HitBoxShape shape;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 direction;
		float radius;
		float height;

		DirectX::XMFLOAT4X4 hitTransform;

		// ヒット時の係数
		float damageRate;
		float poiseRate;
		float invincibleTime;
	};
	// 必要なら書いてもいいよ（デフォルトは空っぽ）っていう書き方
	virtual std::vector<HitBoxInfo> GetActiveHitBoxes() const { return {}; }

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

	// ----- ダメージ / ステータス -----

	// ダメージを与える（無敵時間・体幹ダメージを考慮）
	virtual bool ApplyDamage(float damage, float invincibleTime, float poiseDamage = 0.0f);

	// 体幹削り計算（プレイヤーが呼ぶ用）
	AttackResult CalculateAttackResult(float damageRate, float poiseValue);

	// ステータス更新（毎フレーム呼ぶ）
	void UpdateStatus(float elapsedTime);

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

	// 無敵時間の更新
	void UpdateInvincibleTimer(float elapsedTime);

	// ----- 継承先フック -----

	// 着地したときに呼ばれる
	virtual void OnLanding() {}

	// ダメージを受けた時に呼ばれる
	virtual void OnDamage() {}

	// 死亡した時に呼ばれる
	virtual void OnDead() {}

	// 体幹が削り切られた（ダウンした）時に呼ばれる
	virtual void OnDown() {}

protected:
	// 位置・回転・スケール・行列
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 angle    = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale    = { 1.0f, 1.0f, 1.0f };
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
	float moveVecX     = 0.0f;   // 移動ベクトル（X軸方向）
	float moveVecZ     = 0.0f;   // 移動ベクトル（Z軸方向）

	float friction     = 15.0f;  // 摩擦力
	float acceleration = 50.0f;  // 加速度
	float gravity      = -9.8f;  // 重力

	// 当たり判定
	float radius = 0.5f;  // 半径
	float height = 2.0f;  // 高さ
	float weight = 0.5f;  // 重さ

	bool isGround = false; // 地面と接地しているか

	// ステータス
	float health    = 10.0f; // 現在体力
	float MaxHealth = 10.0f; // 最大体力

	float Stamina    = 150.0f; // 現在持久力
	float MaxStamina = 150.0f; // 最大持久力

	// 体幹（Poise）
	float maxPoise           = 100.0f; // 最大体幹値
	float currentPoise       = 100.0f; // 現在の体幹値
	float baseAttackPower    = 1.0f;   // 基本攻撃力
	float basePoisePower     = 50.0f;  // 基本体幹削り値
	float poiseRecoveryDelay = 0.0f;   // 回復開始までの猶予タイマー
	float poiseFullResetTimer= 0.0f;   // 30秒後に強制全回復用

	// 無敵時間
	float invincibleTimer = 0.0f;

	// ステージの広さ
	float stageRadius = 97.5f;
};