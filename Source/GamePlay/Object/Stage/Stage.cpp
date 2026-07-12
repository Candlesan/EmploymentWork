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

	scale.x = scale.y = scale.z = 0.025f;

	// 	// 頂点データをワールド空間変換し、三角形データを作成
	for (const Model::Mesh& mesh : stage->GetMeshes())
	{
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&mesh.node->worldTransform);
		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			// 頂点データをワールド空間変換
			uint32_t a = mesh.indices.at(i + 0);
			uint32_t b = mesh.indices.at(i + 1);
			uint32_t c = mesh.indices.at(i + 2);
			DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&mesh.vertices.at(a).position);
			DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&mesh.vertices.at(b).position);
			DirectX::XMVECTOR C = DirectX::XMLoadFloat3(&mesh.vertices.at(c).position);
			DirectX::XMVECTOR Scale = DirectX::XMLoadFloat3(&scale);
			A = DirectX::XMVector3Transform(A, WorldTransform);
			B = DirectX::XMVector3Transform(B, WorldTransform);
			C = DirectX::XMVector3Transform(C, WorldTransform);

			// モデルの座標全体に対してスケールを掛けてあげる
			A = DirectX::XMVectorMultiply(A, Scale);
			B = DirectX::XMVectorMultiply(B, Scale);
			C = DirectX::XMVectorMultiply(C, Scale);

			// 法線べクトルを算出
			DirectX::XMVECTOR N = DirectX::XMVector3Cross(DirectX::XMVectorSubtract(B, A),
									DirectX::XMVectorSubtract(C, A));
			if (DirectX::XMVector3Equal(N, DirectX::XMVectorZero()))
			{
				// 面を構成できない場合は除外
				continue;
			}
			N = DirectX::XMVector3Normalize(N);

			// 三角形データを格納
			Collision::Triangle& triangle = triangles.emplace_back();
			DirectX::XMStoreFloat3(&triangle.positions[0], A);
			DirectX::XMStoreFloat3(&triangle.positions[1], B);
			DirectX::XMStoreFloat3(&triangle.positions[2], C);
			DirectX::XMStoreFloat3(&triangle.normal, N);
		}
	}
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
