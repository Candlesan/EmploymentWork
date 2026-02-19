#include "Stage.h"
#include "System/Graphic/Graphics.h"

// コンストラクタ
Stage::Stage()
{
	// デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// ステージモデル読み込み
	stage = std::make_shared<Model>(device, "Data/Model/Stage/circle_of_death.gltf");
	//stage = std::make_shared<Model>(device, "Data/Model/Stage/ExampleStage.gltf");

	scale.x = scale.y = scale.z = 0.01f;
}

// 更新処理
void Stage::Update(float elapsedTime)
{
	// 特に何もすることがないはず
	UpdateTransform();
	stage->UpdateTransform(GetTransform());
}

// 描画処理
void Stage::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	// ゲープロVはDrawで描画をためてRenderで描画する
	renderer->Draw(ShaderId::PBR, stage);
}
