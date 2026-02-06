#include "Stage.h"
#include "System/Graphic/Graphics.h"

// コンストラクタ
Stage::Stage()
{
	// デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// ステージモデル読み込み
	stage = std::make_shared<Model>(device, "Data/Model/Stage/ExampleStage.gltf");
}

// 更新処理
void Stage::Update(float elapsedTime)
{
	// 特に何もすることがないはず
}

// 描画処理
void Stage::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	// ゲープロVはDrawで描画をためてRenderで描画する
	renderer->Draw(ShaderId::Lambert, stage);
}
