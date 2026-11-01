#pragma once
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"
#include "System/Graphic/RenderContext.h"

#include "GamePlay/Object/Collision/Collision.h"

#include <memory>
#include <string>

// 前方宣言
class MagicManager;
class Character;

// 魔法の種類
enum class MagicKind {Normal, Rapid};

// 通常魔法の状態
enum class NormalMagicLevel { Normal, Charged };

// 迎撃用の構造体
struct InterceptProfile
{
	NormalMagicLevel normalLevel = NormalMagicLevel::Normal; // 連射魔法の時は当たらない
	float magicCost = 0.0f;   // 魔力の消費量
	float magicRefund = 0.0f; // 迎撃成功時の返ってくる魔力量
};

class MagicBase
{
public:
	MagicBase(MagicManager* manager);
	virtual~MagicBase(){}
	
	virtual void Update(float elapsedTime) = 0;
	virtual void Render(const RenderContext& rc) = 0;

	// デバッグプリミティブ描画 
	virtual void RenderDebugPrimitive(ShapeRenderer* renderer) = 0;

	// 破棄
	void Destroy();

	//消滅するときのリセット処理
	virtual void OnTerminate() = 0;

	// 位置取得 
	const DirectX::XMFLOAT3& GetPosition() const { return position; }

	// 方向取得 
	const DirectX::XMFLOAT3& GetDirection() const { return direction; }

	// スケール取得 
	const DirectX::XMFLOAT3& GetScale() const { return scale; }

	// トランスフォーム取得
	const DirectX::XMFLOAT4X4& GetTransfor() const { return transform; }

	// 半径取得
	float GetRadius() const { return radius; }
	void SetRadius(float r) { radius = r; }

	// 高さ
	float GetHeight() const { return height; }
	void SetHeight(float h) { height = h; }

	// ダメージ取得
	virtual float GetDamage() const { return damage; }
	void SetDamage(float d) { damage = d; }

	// 形状のゲッター/セッター
	void SetShape(HitBoxShape s) { shape = s; }
	HitBoxShape GetShape() const { return shape; }

	// 魔法の種類のゲッター/セッター
	MagicKind GetMagicKind() const { return magicKind; }
	void SetMagicKind(MagicKind k) { magicKind = k; }

	// 通常魔法の状態
	NormalMagicLevel GetNormalMagicLevel() const { return interceptProfile.normalLevel; }
	void SetNormalMagicLevel(NormalMagicLevel level) { interceptProfile.normalLevel = level; }

	// 迎撃成功時の魔力回復のゲッター
	float GetMagicmagicRefund() const { return interceptProfile.magicRefund; }
	void SetMagicmagicRefund(float regein) { interceptProfile.magicRefund = regein; }

	// 魔法を撃った人
	Character* GetOwner() const { return owner; }
	void SetOwner(Character* o) { owner = o; }

	// 現在生きているか
	bool GetIsAlive() const { return isAlive; }
	void SetIsAlive(bool flag) { isAlive = flag; }

	// 防御魔法へのダメージ
	float GetGuardDamage() const { return guardDamage; }
	void SetGuardDamage(float d) { guardDamage = d; }


	// ヒットエフェクトを出す関数
	void PlayHitEffect(const DirectX::XMFLOAT3& pos);
	void SetHitEffect(const std::string& name, float scale = 1.0f)
	{
		hitEffectName = name;
		hitEffectScale = scale;
	}
	bool GetHitEffectPlayed() const { return hitEffectPlayed; }
	const DirectX::XMFLOAT3& GetPrevPosition() const { return prevPosition; }

	// カメラシェイクを渡す関数
	void SetHitShake(float strength, float duration, float radius);
protected:
	// 行列更新処理 
	void UpdateTransform();

protected:
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 direction = { 0, 0, 1 };
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4 transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	float radius = 1.0f;
	float height = 1.0f;
	float damage = 1.0f;
	float guardDamage = 0.0f;
	bool isAlive = true;

	MagicManager* manager = nullptr;

	HitBoxShape shape = HitBoxShape::Sphere;
	MagicKind magicKind = MagicKind::Normal;
	InterceptProfile interceptProfile;
	Character* owner = nullptr;

	std::string hitEffectName = "";
	float hitEffectScale = 1.0f;
	bool hitEffectPlayed = false;

	DirectX::XMFLOAT3 prevPosition = {};

	float hitShakeStrength = 0.0f; // 0なら揺らさない
	float hitShakeDuration = 0.0f;
	float hitShakeRadius = 0.0f;   // 0以下なら位置なしの揺れ
};