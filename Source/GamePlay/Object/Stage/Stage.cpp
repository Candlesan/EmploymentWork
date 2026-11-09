#include "Stage.h"
#include "System/Graphic/Graphics.h"
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"

#include <imgui.h>


// コンストラクタ
Stage::Stage()
{
	// デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// ステージモデル読み込み
	stage = std::make_shared<Model>(device, "Data/Model/Stage/circle_of_death.gltf");

	scale.x = scale.y = scale.z = 0.025f;

	DirectX::XMVECTOR VolumeMin = DirectX::XMVectorReplicate(FLT_MAX);
	DirectX::XMVECTOR VolumeMax = DirectX::XMVectorReplicate(-FLT_MAX);

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

			// 箱に入る最小点と最大点を保存する
			DirectX::XMStoreFloat3(&triangle.boundsMin, DirectX::XMVectorMin(A, DirectX::XMVectorMin(B, C)));
			DirectX::XMStoreFloat3(&triangle.boundsMax, DirectX::XMVectorMax(A, DirectX::XMVectorMax(B, C)));

			// モデル全体のAABBを計測
			VolumeMin = DirectX::XMVectorMin(VolumeMin, A);
			VolumeMin = DirectX::XMVectorMin(VolumeMin, B);
			VolumeMin = DirectX::XMVectorMin(VolumeMin, C);
			VolumeMax = DirectX::XMVectorMax(VolumeMax, A);
			VolumeMax = DirectX::XMVectorMax(VolumeMax, B);
			VolumeMax = DirectX::XMVectorMax(VolumeMax, C);
		}
	}

	// モデル全体のAABB
	DirectX::XMStoreFloat3(&volumeMin, VolumeMin);
	DirectX::XMStoreFloat3(&volumeMax, VolumeMax);

	// モデル全体のAABBからXZ平面に指定のサイズで分割されたコリジョンエリアを作成する
	{
		const int cellSize = 32;
		for (float x = volumeMin.x; x < volumeMax.x; x += cellSize)
		{
			for (float z = volumeMin.z; z < volumeMax.z; z += cellSize)
			{
				//AABBを算出
				Collision::Area& area = areas.emplace_back();
				//エリアの最大値と最小値を出す
				DirectX::XMFLOAT3 min = { x, volumeMin.y, z };
				DirectX::XMFLOAT3 max = { x + cellSize, volumeMax.y, z + cellSize };

				//バウンディングボックスの中心を出す
				area.boundingBox.Center.x = (min.x + max.x) * 0.5f;
				area.boundingBox.Center.y = (min.y + max.y) * 0.5f;
				area.boundingBox.Center.z = (min.z + max.z) * 0.5f;

				//中心からの各方向の大きさを出す
				area.boundingBox.Extents.x = (max.x - min.x) * 0.5f;
				area.boundingBox.Extents.y = (max.y - min.y) * 0.5f;
				area.boundingBox.Extents.z = (max.z - min.z) * 0.5f;

				//AABBに所属する三角形を抽出
				for (int i = 0; i < static_cast<int>(triangles.size()); ++i)
				{
					Collision::Triangle& tri = triangles[i];
					DirectX::XMFLOAT3& P0 = tri.positions[0];
					DirectX::XMFLOAT3& P1 = tri.positions[1];
					DirectX::XMFLOAT3& P2 = tri.positions[2];

					//三角形の３点を使ってバウンディングボックスを作る
					DirectX::XMFLOAT3 points[3] = { P0, P1, P2 };
					DirectX::BoundingBox triangleBB;
					DirectX::BoundingBox::CreateFromPoints(triangleBB, 3, points, sizeof(DirectX::XMFLOAT3));

					//エリアのAABBと三角形のAABBが交差してたら追加
					if (area.boundingBox.Intersects(triangleBB))
					{
						area.triangleIndices.push_back(i);
					}
				}
			}
		}
	}
}

// 更新処理
void Stage::Update(float elapsedTime)
{
	// 特に何もすることがないはず
	UpdateTransform();
	stage->UpdateTransform(transform); // モデルの行列を更新する
}

// 描画処理
void Stage::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	if(drawStage)
	// Drawで描画をためてRenderで描画する
	renderer->Draw(ShaderId::PBR, stage);
}

// デバックプリミティブ描画
void Stage::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	if(drawArea)
	renderer->DrawCylinder(areaPosition, areaRadius, areaHeight, { 1.0f, 0.5f, 0.5f, 1.0f });
}

// GUI描画
void Stage::DrawGUI()
{
	// F8でステージパラメーターの表示をON/OFF
	static bool wasF8DownLastFrame = false;
	bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
	if (f8Down && !wasF8DownLastFrame)
	{
		showStageParameter = !showStageParameter;
	}
	wasF8DownLastFrame = f8Down;

	if (showStageParameter)
	{
		ImGui::Begin(u8"ステージ");
		ImGui::Checkbox(u8"ステージの表示", &drawStage);
		ImGui::Checkbox(u8"エリアの表示", &drawArea);
		ImGui::Separator();

		ImGui::DragFloat3(u8"エリアの位置", &areaPosition.x, 0.1f);
		ImGui::DragFloat(u8"エリアの半径", &areaRadius, 0.1f);
		ImGui::DragFloat(u8"エリアの高さ", &areaHeight, 0.1f);

		ImGui::End();
	}
}

// 行列更新
void Stage::UpdateTransform()
{
	//スケール行列作成
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	//回転行列を作成
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	//位置行列を作成
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//3つの行列を組み合わせ,ワールド行列を作成
	DirectX::XMMATRIX W = S * R * T;

	//計算したワールド行列を取り出す
	DirectX::XMStoreFloat4x4(&transform, W);
}
