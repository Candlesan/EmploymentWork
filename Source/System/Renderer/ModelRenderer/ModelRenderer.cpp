#include <algorithm>
#include "System/Core/Misc.h"
#include "System/Graphic/GpuResourceUtils.h"
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "System/Shader/Basic/BasicShader.h"
#include "System/Shader/Lambert/LambertShader.h"
#include "System/Shader/PBR/PBRShader.h"
#include "System/Shader/meshVFX/meshVFX.h"

// コンストラクタ
ModelRenderer::ModelRenderer(ID3D11Device* device)
{
	// シーン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbScene),
		sceneConstantBuffer.GetAddressOf());

	// スケルトン用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbSkeleton),
		skeletonConstantBuffer.GetAddressOf());

	// ライト用定数バッファ
	GpuResourceUtils::CreateConstantBuffer(
		device,
		sizeof(CbLight),
		lightConstantBuffer.GetAddressOf());

	// シェーダー生成
	shaders[static_cast<int>(ShaderId::Basic)] = std::make_unique<BasicShader>(device);
	shaders[static_cast<int>(ShaderId::Lambert)] = std::make_unique<LambertShader>(device);
	shaders[static_cast<int>(ShaderId::PBR)] = std::make_unique<PBRShader>(device);
	shaders[static_cast<int>(ShaderId::MeshVFX)] = std::make_unique<meshVFX>(device);
}

// 箱描画
void ModelRenderer::Draw(ShaderId shaderId, std::shared_ptr<Model> model)
{
	DrawInfo& drawInfo = drawInfos.emplace_back();
	drawInfo.shaderId = shaderId;
	drawInfo.model = model;
}

// 描画実行
void ModelRenderer::Render(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// シーン用定数バッファ更新
	{
		static LightManager defaultLightManager;
		const LightManager* lightManager = rc.lightManager ? rc.lightManager : &defaultLightManager;

		CbScene cbScene{};
		DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&rc.camera->GetView());
		DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&rc.camera->GetProjection());
		DirectX::XMStoreFloat4x4(&cbScene.viewProjection, V * P);

		// カメラの逆行列を計算する
		DirectX::XMMATRIX invV = DirectX::XMMatrixInverse(nullptr, V);
		DirectX::XMStoreFloat4x4(&cbScene.inverseView, invV);

		const DirectionalLight& directionalLight = lightManager->GetDirectionalLight();
		cbScene.lightDirection.x = directionalLight.direction.x;
		cbScene.lightDirection.y = directionalLight.direction.y;
		cbScene.lightDirection.z = directionalLight.direction.z;
		cbScene.lightColor.x = directionalLight.color.x * directionalLight.intensity;
		cbScene.lightColor.y = directionalLight.color.y * directionalLight.intensity;
		cbScene.lightColor.z = directionalLight.color.z * directionalLight.intensity;
		const DirectX::XMFLOAT3& eye = rc.camera->GetEye();
		cbScene.cameraPosition.x = eye.x;
		cbScene.cameraPosition.y = eye.y;
		cbScene.cameraPosition.z = eye.z;
		dc->UpdateSubresource(sceneConstantBuffer.Get(), 0, 0, &cbScene, 0, 0);

		// ライト用定数バッファ更新
		{
			CbLight cbLight{};

			auto& pushPoint = [&](const PointLight& p)
			{
				if (!p.enabled || p.range <= 0.0f) return; // ライトが点いているまたは長さが0以下の場合は飛ばす
				if (cbLight.pointLightCount >= LightManager::MaxPointLights) return; // 置けるライトの最大数を超えていたら

				CbPointLight& dst = cbLight.pointLights[cbLight.pointLightCount++];
				dst.position = { p.position.x, p.position.y, p.position.z, p.range };
				dst.color = { p.color.x * p.intensity, p.color.y * p.intensity, p.color.z * p.intensity, 0.0f };

			};

			// 点光源
			for (const PointLight& p : lightManager->GetFramePointLights()) pushPoint(p);
			for (const PointLight& p : lightManager->GetPointLights()) pushPoint(p);

			// スポットライト
			for (const SpotLight& s : lightManager->GetSpotLights())
			{
				if (!s.enabled || s.range <= 0.0f) continue; // ライトが点いているまたは長さが0以下の場合はやり直し
				if (cbLight.spotLightCount >= LightManager::MaxSpotLights) break; // 置けるライトの最大数を超えていたら

				DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&s.direction);
				if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(dir)) < 1e-6f) continue; // 向きが0なら飛ばす
				DirectX::XMFLOAT3 d;
				DirectX::XMStoreFloat3(&d, DirectX::XMVector3Normalize(dir));

				// インナー < アウターを保証(等しいとき0除算)
				float inner = (std::max)(s.innerAngle, 0.0f);
				float outer = (std::max)(s.outerAngle, inner + 0.5f);

				CbSpotLight& dst = cbLight.spotLights[cbLight.spotLightCount++];
				dst.position = { s.position.x, s.position.y, s.position.z, s.range };
				dst.direction = { d.x, d.y, d.z, 0.0f };
				dst.color = { s.color.x * s.intensity, s.color.y * s.intensity, s.color.z * s.intensity, 0.0f };
				dst.cone = {
					cosf(DirectX::XMConvertToRadians(inner)),
					cosf(DirectX::XMConvertToRadians(outer)),
					0.0f, 0.0f };
			}

			dc->UpdateSubresource(lightConstantBuffer.Get(), 0, 0, &cbLight, 0, 0);
		}
	}

	// 定数バッファ設定
	ID3D11Buffer* vsConstantBuffers[] =
	{
		skeletonConstantBuffer.Get(),
		sceneConstantBuffer.Get(),
	};
	ID3D11Buffer* psConstantBuffers[] =
	{
		sceneConstantBuffer.Get(),
	};
	dc->VSSetConstantBuffers(6, _countof(vsConstantBuffers), vsConstantBuffers);
	dc->PSSetConstantBuffers(7, _countof(psConstantBuffers), psConstantBuffers);
	dc->PSSetConstantBuffers(12, 1, lightConstantBuffer.GetAddressOf());

	// サンプラステート設定
	ID3D11SamplerState* samplerStates[] =
	{
		rc.renderState->GetSamplerState(SamplerState::LinearWrap)
	};
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);

	// レンダーステート設定
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);


	dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// メッシュ描画関数
	auto drawMesh = [&](const Model::Mesh& mesh, Shader* shader)
	{
		// 頂点バッファ設定
		UINT stride = sizeof(Model::Vertex);
		UINT offset = 0;
		dc->IASetVertexBuffers(0, 1, mesh.vertexBuffer.GetAddressOf(), &stride, &offset);
		dc->IASetIndexBuffer(mesh.indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// スケルトン用定数バッファ更新
		CbSkeleton cbSkeleton{};
		if (mesh.bones.size() > 0)
		{
			for (size_t i = 0; i < mesh.bones.size(); ++i)
			{
				const Model::Bone& bone = mesh.bones.at(i);
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&bone.node->worldTransform);
				DirectX::XMMATRIX OffsetTransform = DirectX::XMLoadFloat4x4(&bone.offsetTransform);
				DirectX::XMMATRIX BoneTransform = OffsetTransform * WorldTransform;
				DirectX::XMStoreFloat4x4(&cbSkeleton.boneTransforms[i], BoneTransform);
			}
		}
		else
		{
			cbSkeleton.boneTransforms[0] = mesh.node->worldTransform;
		}
		dc->UpdateSubresource(skeletonConstantBuffer.Get(), 0, 0, &cbSkeleton, 0, 0);

		// 更新
		shader->Update(rc, mesh);

		// 描画
		dc->DrawIndexed(static_cast<UINT>(mesh.indices.size()), 0, 0);
	};

	DirectX::XMVECTOR CameraPosition = DirectX::XMLoadFloat3(&rc.camera->GetEye());
	DirectX::XMVECTOR CameraFront = DirectX::XMLoadFloat3(&rc.camera->GetFront());

	// ブレンドステート設定
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);

	// 不透明描画処理
	for (DrawInfo& drawInfo : drawInfos)
	{
		Shader* shader = shaders[static_cast<int>(drawInfo.shaderId)].get();
		shader->Begin(rc);

		for (const Model::Mesh& mesh : drawInfo.model->GetMeshes())
		{
			// 半透明メッシュ登録
			if (mesh.material->alphaMode == Model::AlphaMode::Blend ||
				(mesh.material->baseColor.w > 0.01f && mesh.material->baseColor.w < 0.99f))
			{
				TransparencyDrawInfo& transparencyDrawInfo = transparencyDrawInfos.emplace_back();
				transparencyDrawInfo.shaderId = drawInfo.shaderId;
				transparencyDrawInfo.mesh = &mesh;
				// カメラとの距離を算出
				DirectX::XMVECTOR Position = DirectX::XMVectorSet(
					mesh.node->worldTransform._41,
					mesh.node->worldTransform._42,
					mesh.node->worldTransform._43,
					0.0f);
				DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Position, CameraPosition);
				transparencyDrawInfo.distance = DirectX::XMVectorGetX(DirectX::XMVector3Dot(CameraFront, Vec));

				continue;
			}

			// 服とか髪用
			if (mesh.material->alphaMode == Model::AlphaMode::Mask ||
				mesh.material->alphaMode == Model::AlphaMode::Blend)
			{
				// 裏面も描画する
				dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));
			}
			else
			{
				// 通常のパーツは背面をカット（描画負荷軽減）
				dc->RSSetState(rc.renderState->GetRasterizerState(
					rc.isShadowPass ? RasterizerState::SolidCullBack : RasterizerState::SolidCullNone));
			}

			// 描画
			drawMesh(mesh, shader);
		}

		shader->End(rc);
	}
	drawInfos.clear();

	// ブレンドステート設定
	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);

	// カメラから遠い順にソート
	std::sort(transparencyDrawInfos.begin(), transparencyDrawInfos.end(),
		[](const TransparencyDrawInfo& lhs, const TransparencyDrawInfo& rhs)
		{
			return lhs.distance > rhs.distance;
		});

	// 半透明描画処理
	for (const TransparencyDrawInfo& transparencyDrawInfo : transparencyDrawInfos)
	{
		Shader* shader = shaders[static_cast<int>(transparencyDrawInfo.shaderId)].get();

		shader->Begin(rc);

		// 半透明も裏側が見えるように設定
		dc->RSSetState(rc.renderState->GetRasterizerState(RasterizerState::SolidCullNone));

		drawMesh(*transparencyDrawInfo.mesh, shader);

		shader->End(rc);
	}
	transparencyDrawInfos.clear();

	// 定数バッファ設定解除
	for (ID3D11Buffer*& vsConstantBuffer : vsConstantBuffers) { vsConstantBuffer = nullptr; }
	for (ID3D11Buffer*& psConstantBuffer : psConstantBuffers) { psConstantBuffer = nullptr; }
	dc->VSSetConstantBuffers(6, _countof(vsConstantBuffers), vsConstantBuffers);
	dc->PSSetConstantBuffers(7, _countof(psConstantBuffers), psConstantBuffers);
	ID3D11Buffer* nullBuffer = nullptr;
	dc->PSSetConstantBuffers(12, 1, &nullBuffer);

	// サンプラステート設定解除
	for (ID3D11SamplerState*& samplerState : samplerStates) { samplerState = nullptr; }
	dc->PSSetSamplers(0, _countof(samplerStates), samplerStates);
}