#pragma once
#include <DirectXMath.h>
#include "System/Renderer/ModelRenderer.h"
#include "System/Renderer/ShapeRenderer.h"

struct AttackResult {
	float damage;
	float poiseDamage;
};

class Character
{
public:
	Character() {}
	virtual ~Character() = default;

	// 行列更新
	void UpdateTransform();

	// 位置取得
	const DirectX::XMFLOAT3& GetPosition()const { return position; }

	// 位置設定
	void SetPosition(const DirectX::XMFLOAT3& position) { this->position = position; }

	// 回転取得
	const DirectX::XMFLOAT3& GetAngle()const { return angle; }

	// 回転設定
	void SetAngle(const DirectX::XMFLOAT3& angle) { this->angle = angle; }

	// スケール取得
	const DirectX::XMFLOAT3& GetScale()const { return scale; }

	// スケール取得
	void SetScale(const DirectX::XMFLOAT3& scale) { this->scale = scale; }

	// 行列取得
	const DirectX::XMFLOAT4X4& GetTransform() const { return transform; }

	// 行列設定
	void Set_Transform(DirectX::XMFLOAT4X4 new_transform) { this->transform = new_transform; }

	// 速力取得
	const DirectX::XMFLOAT3& GetVelocity() const { return velocity; }

	// 速力設定
	void SetVelocity(const DirectX::XMFLOAT3& v) { velocity = v; }

	// 半径取得
	float GetRadius() const { return radius; }

	// 高さ取得
	float GetHeight() const { return height; }

	// 重さ取得
	float GetWeight() const { return weight; }

	// 地面に接地しているか
	bool IsGround() const { return isGround; }

	// 接地フラグ取得
	bool SetIsGround(bool flag) { return isGround = flag; }

	// ダメージを与える
	virtual bool ApplyDamage(float damage, float invincibleTime, float poiseDamage = 0.0f);

	// 移動処理
	void Move(float vx, float vz, float speed);

	// 旋回処理
	void Turn(float elapsedTime, float vx, float vz, float speed);

	// カプセルの方向取得
	DirectX::XMFLOAT3 GetCapsuleDirection() const
	{ return DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f); }

	// 体幹削り計算用（プレイヤーが呼ぶ用）
	AttackResult CalculateAttackResult(float damageRate, float poiseValue);

	// ステータス更新
	void UpdateStatus(float elapsedTime);

	// エリア制限
	void AreaRestriction();
private:
	// 水平速力更新処理
	void UpdateHorizontalVelocity(float elapsedTime);

	// 水平移動更新処理
	void UpdateHorizontalMove(float elapsedTime);

	//垂直速力更新処理
	void UpdateVerticalVelocity(float elapsedTime);

	//垂直移動更新処理
	void UpdateVerticalMove(float elapsedTime);

protected:

	// ジャンプ処理
	void Jump(float speed);

	// 速力処理更新
	void UpdateVelocity(float elapsedTime);

	// 着地したときに呼ばれる
	virtual void OnLanding() {}

	// ダメージを受けた時に呼ばれる
	virtual void OnDamage() {}

	// 死亡した時に呼ばれる
	virtual void OnDead() {}

	// 体幹が削りきられた（ダウンした）時に呼ばれる仮想関数
	virtual void OnDown() {}

	// 無敵時間更新
	void UpdateInvincibleTimer(float elapsedTime);


protected:
	DirectX::XMFLOAT3 position = { 0,0,0 };
	DirectX::XMFLOAT3 angle = { 0,0,0 };
	DirectX::XMFLOAT3 scale = { 1,1,1 };
	DirectX::XMFLOAT4X4 transform = {
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,
		0,0,0,1
	};

	DirectX::XMFLOAT3 velocity = { 0, 0, 0 }; 

	float maxMoveSpeed = 5.0f; // 最大速度
	float moveVecX = 0.0f; // 移動ベクトル（X軸方向）
	float moveVecZ = 0.0f; // 移動ベクトル（Z軸方向）

	float friction = 15.0f; // 摩擦力
	float acceleration = 50.0f; // 加速度
	float gravity = -9.8; // 重力

	//当たり判定関係
	float radius = 0.5f; // 半径
	float height = 2.0f; // 高さ
	float weight = 0.5f; // 重さ

	bool isGround = false; // 地面と接地してるか

	float health = 10; // 体力
	float MaxHealth = 10; // 最大体力

	float Stamina = 150; // 持久力
	float MaxStamina = 150; // 最大持久力

	float maxPoise = 100.0f;        // 最大体幹値
	float currentPoise = 100.0f;    // 現在の体幹値

	float baseAttackPower = 1; // 基本攻撃力
	float basePoisePower = 50.0f;    // 基本体幹削り値
	float poiseRecoveryDelay = 0.0f;  // 回復が始まるまでの猶予タイマー
	float poiseFullResetTimer = 0.0f; // 30秒後に強制全回復する用

	float invincibleTimer = 0.0f; // 無敵時間

	float stageRadius = 38.0f;
};