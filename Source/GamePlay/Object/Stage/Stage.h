#pragma once
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "GamePlay/Object/Collision/Collision.h"

#include <memory>

// 前方宣言
class ShapeRenderer;

// ステージ
class Stage
{
public:
	Stage();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	// デバック描画
	void RenderDebugPrimitive(ShapeRenderer* renderer);

	// GUI描画
	void DrawGUI();

	// 行列更新
	void UpdateTransform();

	const std::vector<Collision::Triangle>& GetTriangles() const { return triangles; }
	const std::vector<Collision::Area>& GetAreas() const { return areas; }

	const DirectX::XMFLOAT3& GetPosition() const { return areaPosition; } // 移動範囲の中止位置
	float GetRadius() const { return areaRadius; } // 移動範囲の半径
	float GetHeight() const { return areaHeight; } // 移動範囲の高さ

	const DirectX::XMFLOAT3& GetVolumeMin() const { return volumeMin; }
	const DirectX::XMFLOAT3& GetVolumeMax() const { return volumeMax; }
private:
	std::shared_ptr<Model> stage;
	std::vector<Collision::Triangle> triangles;
	std::vector<Collision::Area> areas;

	DirectX::XMFLOAT4X4 transform = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 angle = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	bool drawStage = true;

	// ステージの大きさ
	DirectX::XMFLOAT3 areaPosition = { 0.0f, -10.0f, 0.0f }; // 移動範囲の位置
	float areaRadius = 98.0f; // 移動範囲の半径
	float areaHeight = 60.0f; // 移動範囲の高さ
	bool drawArea = true;

	bool showStageParameter = true;

	DirectX::XMFLOAT3 volumeMin;
	DirectX::XMFLOAT3 volumeMax;
};
