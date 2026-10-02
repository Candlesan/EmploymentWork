#pragma once

#include <memory>
#include <vector>
#include <wrl.h>
#include <d3d11.h>
#include <DirectXMath.h>
#include "System/Resource/Model/Model.h"
#include "System/Shader/Shader.h"

enum class ShaderId
{
	Basic,
	Lambert,
	PBR,
	MeshVFX,

	EnumCount
};

class ModelRenderer
{
public:
	ModelRenderer(ID3D11Device* device);
	~ModelRenderer() {}

	// 箱描画
	void Draw(ShaderId shaderId, std::shared_ptr<Model> model);

	// 描画実行
	void Render(const RenderContext& rc);

	// 指定した ShaderId の Shader ポインタを取得するゲッターを追加！
	Shader* GetShader(ShaderId shaderId)
	{
		int index = static_cast<int>(shaderId);
		if (index >= 0 && index < static_cast<int>(ShaderId::EnumCount))
		{
			return shaders[index].get();
		}
		return nullptr;
	}
private:
	struct CbScene
	{
		DirectX::XMFLOAT4X4		viewProjection;
		DirectX::XMFLOAT4		lightDirection;
		DirectX::XMFLOAT4		lightColor;
		DirectX::XMFLOAT4		cameraPosition;
		DirectX::XMFLOAT4X4		inverseView;
	};

	struct CbSkeleton
	{
		DirectX::XMFLOAT4X4		boneTransforms[256];
	};

	struct DrawInfo
	{
		ShaderId				shaderId;
		std::shared_ptr<Model>	model;
		float					distance;
	};

	struct TransparencyDrawInfo
	{
		ShaderId				shaderId;
		const Model::Mesh*		mesh;
		float					distance;
	};

	std::unique_ptr<Shader>					shaders[static_cast<int>(ShaderId::EnumCount)];
	std::vector<DrawInfo>					drawInfos;
	std::vector<TransparencyDrawInfo>		transparencyDrawInfos;

	Microsoft::WRL::ComPtr<ID3D11Buffer>	sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>	skeletonConstantBuffer;
};
