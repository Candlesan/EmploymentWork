#pragma once
#include "System/Renderer/ModelRenderer.h"
#include "GamePlay/Object/Character/Character.h"

#include <memory>


// ステージ
class Stage : public Character
{
public:
	Stage();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	const std::vector<Collision::Triangle>& GetTriangles() const { return triangles; }
	const std::vector<Collision::Area>& GetAreas() const { return areas; }

private:
	std::shared_ptr<Model> stage;
	std::vector<Collision::Triangle> triangles;
	std::vector<Collision::Area> areas;
};
