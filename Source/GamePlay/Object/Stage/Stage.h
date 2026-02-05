#pragma once
#include "System/Renderer/ModelRenderer.h"
#include <memory>


// ステージ
class Stage
{
public:
	Stage();

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

private:
	std::shared_ptr<Model> stage;
};
