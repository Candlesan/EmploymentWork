#include "Trail.h"
#include "System//Graphic/Graphics.h"

void Trail::Update(DirectX::XMFLOAT3 root, DirectX::XMFLOAT3 tip)
{
	// 過去の軌跡を一つ後ろにずらす
	for (int i = MAX_POLYGON - 1; i > 0; --i)
	{
		trailPositions[0][i] = trailPositions[0][i - 1]; // 根本を一つ後ろへ
		trailPositions[1][i] = trailPositions[1][i - 1]; // 先端を一つ後ろへ
	}

	// 剣の根元の座標を取得し最新の頂点座標を保存
	trailPositions[0][0] = root;
	trailPositions[1][0] = tip;
}

void Trail::Render()
{
	// ポリゴン作成
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	if (!spline)
	{
		// 保存していた頂点バッファでポリゴンを作る
		for (int i = 0; i < MAX_POLYGON; ++i)
		{
			primitiveRenderer->AddVertex(trailPositions[0][i], color);
			primitiveRenderer->AddVertex(trailPositions[1][i], color);
		}
	}
	else
	{
		const int segments = 8;

		for (int i = 1; i < MAX_POLYGON - 2; ++i) // ちゃんとp0～p3が揃う範囲
		{
			// 根元側と先端側の4点を取得（正しい順番）
			DirectX::XMVECTOR rootP0 = DirectX::XMLoadFloat3(&trailPositions[0][i - 1]);
			DirectX::XMVECTOR rootP1 = DirectX::XMLoadFloat3(&trailPositions[0][i]);
			DirectX::XMVECTOR rootP2 = DirectX::XMLoadFloat3(&trailPositions[0][i + 1]);
			DirectX::XMVECTOR rootP3 = DirectX::XMLoadFloat3(&trailPositions[0][i + 2]);

			DirectX::XMVECTOR tipP0 = DirectX::XMLoadFloat3(&trailPositions[1][i - 1]);
			DirectX::XMVECTOR tipP1 = DirectX::XMLoadFloat3(&trailPositions[1][i]);
			DirectX::XMVECTOR tipP2 = DirectX::XMLoadFloat3(&trailPositions[1][i + 1]);
			DirectX::XMVECTOR tipP3 = DirectX::XMLoadFloat3(&trailPositions[1][i + 2]);

			for (int j = 0; j < segments; ++j)
			{
				float t = static_cast<float>(j) / segments;

				// スプライン補完（正しい順）
				DirectX::XMVECTOR rootPos = DirectX::XMVectorCatmullRom(rootP0, rootP1, rootP2, rootP3, t);
				DirectX::XMVECTOR tipPos = DirectX::XMVectorCatmullRom(tipP0, tipP1, tipP2, tipP3, t);

				DirectX::XMFLOAT3 rootResult, tipResult;
				DirectX::XMStoreFloat3(&rootResult, rootPos);
				DirectX::XMStoreFloat3(&tipResult, tipPos);

				primitiveRenderer->AddVertex(rootResult, color);
				primitiveRenderer->AddVertex(tipResult, color);
			}
		}
	}
}
