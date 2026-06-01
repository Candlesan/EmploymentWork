#pragma once
#include "System/Renderer/ShapeRenderer.h"
#include "System/Graphic/RenderContext.h"

#include <memory>

// 前方宣言
class MagicManager;

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

	// 位置取得 
	const DirectX::XMFLOAT3& GetPosition() const { return position; }

	// 方向取得 
	const DirectX::XMFLOAT3& GetDirection() const { return direction; }

	// スケール取得 
	const DirectX::XMFLOAT3& GetScale() const { return scale; }

	// 半径取得
	float GetRadius() const { return radius; }

protected:
	// 行列更新処理 
	void UpdateTransform();

protected:
	DirectX::XMFLOAT3 position = { 0, 0, 0 };
	DirectX::XMFLOAT3 direction = { 0, 0, 1 };
	DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	DirectX::XMFLOAT4X4 transform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	float radius = 1.0f;

	MagicManager* manager = nullptr;
};