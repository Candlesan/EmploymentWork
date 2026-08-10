#pragma once
#include "System/Renderer/ShapeRenderer.h"
#include "System/Graphic/RenderContext.h"

#include "GamePlay/Object/Collision/Collision.h"

#include <memory>

// 前方宣言
class MagicManager;

// 魔法の種類
enum class MagicKind {Normal, Rapid};

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

	bool GetIsAlive() const { return isAlive; }
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
	bool isAlive = true;

	MagicManager* manager = nullptr;

	HitBoxShape shape = HitBoxShape::Sphere;
	MagicKind magicKind = MagicKind::Normal;
};