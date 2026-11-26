#include "SceneRenderEnv.h"

#include "System/Graphic/Graphics.h"
#include "System/Graphic/GpuResourceUtils.h"

#include "System/Shader/fullscreen_quad/fullscreen_quad.h"
#include "System/Shader/FrameBuffer/FrameBuffer.h"
#include "System/Shader/SkyMap/SkyMap.h"
#include "System/Shader/bloom/bloom.h"


#include <imgui.h>


SceneRenderEnv::SceneRenderEnv() = default;
SceneRenderEnv::~SceneRenderEnv() = default;

// 初期化
void SceneRenderEnv::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// ライト設定
	DirectionalLight light;

	light.direction = CalcLightDirection(); // 斜め上から
	light.color = { 0.25f, 0.8f, 0.9f }; // 青白い色
	light.intensity = 0.7f;

	lightManager.SetDirectionalLight(light);
	lightManager.Load(LightManager::LightsFilePath);

	// スカイマップ初期化
	skyMap = std::make_unique<SkyMap>(Graphics::Instance().GetDevice());

	// ステージの影の設定
	stageShadowAttenuation = 0;
	stageShadowBias = 0.001f;

	// フレームバッファ
	framebuffers[0] = std::make_unique<framebuffer>(device, screenWidth, screenHeight, true);
	bit_block_transfer = std::make_unique<fullscreen_quad>(device);

	// ブルーム関連
	bloomer = std::make_unique<bloom>(device, screenWidth, screenHeight);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/final_pass_ps.cso", pixel_shaders[0].ReleaseAndGetAddressOf());
	bloomer->bloom_intensity = bloom_intensity;
	bloomer->bloom_extraction_threshold = bloom_extraction;
	bloomer->exposure = bloom_exposure;
}

// 描画
void SceneRenderEnv::SkyRender(const RenderContext& rc)
{
	skyMap->Draw(rc); // スカイマップ描画
}

// ステージの影を描画
void SceneRenderEnv::RenderStageShadow(
	const DirectX::XMFLOAT3& volumeMin, 
	const DirectX::XMFLOAT3& volumeMax, 
	const std::function<void(const RenderContext&)>& drawCasters)
{
	if (stageShadowRendered) return;
		stageShadowRendered = true;

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	Camera& camera = CameraManager::Instance().GetMainCamera();
	Graphics& graphics = Graphics::Instance();
	ShadowMap* shadowMap = graphics.GetStageShadowMap();

	// レンダーステート設定
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullBack));

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;
	rc.isShadowPass = true;

	// カメラをライトから見た情報に変更
	{
		// 最初のカメラの情報を保存する
		camera.SaveCamera();


		// ライト方向を取得
		DirectX::XMFLOAT3 lightDir = lightManager.GetDirectionalLight().direction;

		// ステージの大きさに収める
		DirectX::XMFLOAT3 vmin = volumeMin;
		DirectX::XMFLOAT3 vmax = volumeMax;

		// ステージの大きさを計算する
		DirectX::XMVECTOR Min = DirectX::XMLoadFloat3(&vmin);
		DirectX::XMVECTOR Max = DirectX::XMLoadFloat3(&vmax);
		DirectX::XMVECTOR Center = DirectX::XMVectorScale(DirectX::XMVectorAdd(Min, Max), 0.5f);
		stageRadius = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(Max, Center)));

		DirectX::XMVECTOR L = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&lightDir));
		DirectX::XMFLOAT3 lightPos, lightTarget;
		DirectX::XMStoreFloat3(&lightPos, DirectX::XMVectorSubtract(Center, DirectX::XMVectorScale(L, stageRadius)));
		DirectX::XMStoreFloat3(&lightTarget, Center);

		DirectX::XMFLOAT3 lightEyeUp = { 0, 1, 0 }; // 上方向を向く

		// 計算した内容をセットする
		camera.SetLookAt(lightPos, lightTarget, lightEyeUp);
		camera.SetOrthographic(
			stageRadius * 1.8f,   // 横幅：アリーナの直径
			stageRadius * 1.8f,   // 高さ
			0.0f,				  // 近平面：ライト位置から
			stageRadius * 1.8f    // 遠平面：アリーナの反対側まで
		);

		DirectX::XMFLOAT4X4 view = camera.GetView();
		DirectX::XMFLOAT4X4 projaection = camera.GetProjection();
		DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
		DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projaection);

		// ライトビュープロジェクション行列を計算して保存
		// シャドウマップ用の情報に入れる
		DirectX::XMMATRIX lightViewProjection_M = DirectX::XMMatrixMultiply(V, P);
		DirectX::XMStoreFloat4x4(&stageLightViewProjection, lightViewProjection_M);
	}

	shadowMap->Active(dc);
	{
		drawCasters(rc);
	}
	shadowMap->Deactive(dc);

	//変更したカメラの情報を元に戻す
	camera.ResetCamera();
}

// GUI描画
void SceneRenderEnv::DrawGUI()
{
	Graphics& graphics = Graphics::Instance();
	ShadowMap* stageShadowMap = graphics.GetStageShadowMap();

	if (ImGui::CollapsingHeader(u8"ライト", ImGuiTreeNodeFlags_DefaultOpen))
	{
		 DirectionalLight& light = lightManager.GetDirectionalLight();

		// 方位と高さは別々に動く（お互いに影響しない）
		 if (ImGui::SliderFloat(u8"ライトの方向", &lightYaw, 0.0f, 360.0f)) InvalidateShadow();
		 if (ImGui::SliderFloat(u8"ライトの高さ", &lightPitch, 5.0f, 89.0f)) InvalidateShadow();
		 ImGui::DragFloat(u8"ライトの強さ", &light.intensity, 0.1f, 0.0f, 10.0f);
		light.direction = CalcLightDirection();

		// 確認用：実際の向き
		ImGui::Text(u8"向き: %.3f, %.3f, %.3f", light.direction.x, light.direction.y, light.direction.z);

		ImGui::ColorEdit3(u8"ライトの色", &light.color.x);
	}

	if (ImGui::Button(u8"ライト保存"))       lightManager.Save(LightManager::LightsFilePath);
	ImGui::SameLine();
	if (ImGui::Button(u8"ライト再読み込み")) lightManager.Load(LightManager::LightsFilePath);

	if (ImGui::CollapsingHeader(u8"点光源"))
	{
		auto& points = lightManager.GetPointLights();
		if (ImGui::Button(u8"追加##point") && points.size() < LightManager::MaxPointLights)
		{
			points.emplace_back();
		}
		ImGui::SameLine();
		ImGui::Text(u8"%d / %d", (int)points.size(), LightManager::MaxPointLights);

		for (int i = 0; i < (int)points.size(); ++i)
		{
			ImGui::PushID(i);
			PointLight& p = points[i];
			bool remove = false;
			if (ImGui::TreeNode("point", u8"点光源 %d", i))
			{
				ImGui::Checkbox(u8"有効", &p.enabled);
				ImGui::DragFloat3(u8"位置", &p.position.x, 0.1f);
				ImGui::ColorEdit3(u8"色", &p.color.x);
				ImGui::DragFloat(u8"強さ", &p.intensity, 0.05f, 0.0f, 50.0f);
				ImGui::DragFloat(u8"範囲", &p.range, 0.05f, 0.1f, 100.0f);
				remove = ImGui::Button(u8"削除");
				ImGui::TreePop();
			}
			ImGui::PopID();
			if (remove)
			{
				points.erase(points.begin() + i);
				break;   // 配列が変わったのでこのフレームはここで抜ける
			}
		}
	}

	if (ImGui::CollapsingHeader(u8"スポットライト"))
	{
		auto& spots = lightManager.GetSpotLights();
		if (ImGui::Button(u8"追加##spot") && spots.size() < LightManager::MaxSpotLights)
		{
			spots.emplace_back();
		}
		ImGui::SameLine();
		ImGui::Text(u8"%d / %d", (int)spots.size(), LightManager::MaxSpotLights);

		for (int i = 0; i < (int)spots.size(); ++i)
		{
			ImGui::PushID(i + 1000);
			SpotLight& s = spots[i];
			bool remove = false;
			if (ImGui::TreeNode("spot", u8"スポット %d", i))
			{
				ImGui::Checkbox(u8"有効", &s.enabled);
				ImGui::DragFloat3(u8"位置", &s.position.x, 0.1f);
				ImGui::DragFloat3(u8"向き", &s.direction.x, 0.01f, -1.0f, 1.0f);
				ImGui::ColorEdit3(u8"色", &s.color.x);
				ImGui::DragFloat(u8"強さ", &s.intensity, 0.05f, 0.0f, 50.0f);
				ImGui::DragFloat(u8"範囲", &s.range, 0.05f, 0.1f, 200.0f);
				ImGui::SliderFloat(u8"インナー角", &s.innerAngle, 0.0f, 89.0f, u8"%.1f°");
				ImGui::SliderFloat(u8"アウター角", &s.outerAngle, s.innerAngle, 89.0f, u8"%.1f°");
				remove = ImGui::Button(u8"削除");
				ImGui::TreePop();
			}
			ImGui::PopID();
			if (remove)
			{
				spots.erase(spots.begin() + i);
				break;
			}
		}
	}

	if (ImGui::CollapsingHeader(u8"PBR（物理ベース）", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat(u8"金属度", &pbrMetalness, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat(u8"粗さ", &pbrRoughness, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat(u8"環境光の強さ", &ambientIntensity, 0.1f, 0.0f, 10.0f);
	}

	if (ImGui::CollapsingHeader(u8"スカイマップ", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat(u8"空の明るさ", &skyMap->sky_intensity, 0.001f, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader(u8"ステージの影", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat(u8"影の減衰##stage", &stageShadowAttenuation, 0.001f, 0.0f, 1.0f);
		ImGui::DragFloat(u8"シャドウバイアス##stage", &stageShadowBias, 0.001f, 0.0f, 1.0f);

		ImGui::Text(u8"テクスチャ##stage");
		ImGui::Image(stageShadowMap->GetShaderResourceView(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });
	}

	if (ImGui::CollapsingHeader(u8"ブルーム", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat(u8"ブローム抽出閾値", &bloom_extraction, 0.01f, +0.0f, +5.0f);
		ImGui::DragFloat(u8"ブローム強度", &bloom_intensity, 0.01f, +0.0f, +5.0f);
		ImGui::DragFloat(u8"露出", &bloom_exposure, 0.01f, 0.0f, 1.0f);

		bloomer->bloom_extraction_threshold = bloom_extraction;
		bloomer->bloom_intensity = bloom_intensity;
		bloomer->exposure = bloom_exposure;

		ImGui::Separator();

		// グロー抽出
		ImGui::Text(u8"グロー抽出");
		ImGui::Image((ImTextureID)bloomer->get_glkow_extraction(), ImVec2(256, 256));

		// ガウシアンブラー各段階
		for (size_t i = 0; i < bloom::downsampled_count; ++i)
		{
			ImGui::Text(u8"ガウシアンブラー [%zu][0]", i);
			ImGui::Image((ImTextureID)bloomer->get_gaussian_blur(i, 0), ImVec2(256, 256));
			ImGui::Text(u8"ガウシアンブラー [%zu][1]", i);
			ImGui::Image((ImTextureID)bloomer->get_gaussian_blur(i, 1), ImVec2(256, 256));
		}
	}
}

// 画面比率修正
void SceneRenderEnv::OnResize(float w, float h)
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	framebuffers[0] = std::make_unique<framebuffer>(device, w, h, true);

	// ブルームを新しいサイズで丸ごと生成し直す
	bloomer = std::make_unique<bloom>(device, w, h);

	// 初期化時に設定していたパラメータを再設定
	bloomer->bloom_intensity = bloom_intensity;
	bloomer->bloom_extraction_threshold = bloom_extraction;
	bloomer->exposure = bloom_exposure;
}

// ポストエフェクト開始
void SceneRenderEnv::BeginScene(const RenderContext& rc)
{
	framebuffers[0]->clear(rc.deviceContext);
	framebuffers[0]->activate(rc.deviceContext);
}

// ポストエフェクト終了
void SceneRenderEnv::EndScene(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	framebuffers[0]->deactivate(dc);
	bloomer->make(dc, framebuffers[0]->shader_resource_views[0].Get(), rc.renderState);
}

// 2枚のテクスチャを送る
void SceneRenderEnv::Present(const RenderContext& rc)
{
	ID3D11DeviceContext* dc = rc.deviceContext;

	// ブルーム設定
	ID3D11ShaderResourceView* shader_resource_views[] =
	{
		framebuffers[0]->shader_resource_views[0].Get(),
		bloomer->shader_resource_view(),
	};

	Graphics::Instance().SetRenderTargets();

	dc->OMSetBlendState(rc.renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(rc.renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

	bloomer->set_constant_buffer(dc);
	bit_block_transfer->blit(dc, shader_resource_views, 0, 2, pixel_shaders[0].Get());
}

// 描画情報を設定
void SceneRenderEnv::SetupRenderContext(RenderContext& rc)
{
	// PBR
	rc.pbrMetalness = pbrMetalness;
	rc.pbrRoughness = pbrRoughness;
	rc.ambientIntensity = ambientIntensity;

	// ライト
	rc.lightManager = &lightManager;

	// stage用のャドウマップ準備
	ShadowMap* shadowMap = Graphics::Instance().GetStageShadowMap();
	stageShadowData.shadowMap = shadowMap->GetShaderResourceView();
	stageShadowData.shadowSampler = shadowMap->GetSamplerState();
	stageShadowData.lightViewProjection = stageLightViewProjection;
	stageShadowData.shadowColor = DirectX::XMFLOAT4(0, 0, 0, 0.1);
	stageShadowData.shadowBias = stageShadowBias;
	stageShadowData.shadowAttenuation = stageShadowAttenuation;
	rc.stageShadowMapData = &stageShadowData;
}

// 光の方向を計算
DirectX::XMFLOAT3 SceneRenderEnv::CalcLightDirection() const
{
	float Yaw = DirectX::XMConvertToRadians(lightYaw);
	float Pitch = DirectX::XMConvertToRadians(lightPitch);
	return {
		cosf(Pitch) * sinf(Yaw),
		-sinf(Pitch),
		cosf(Pitch) * cosf(Yaw)
	};
}