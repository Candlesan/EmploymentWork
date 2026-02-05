#include "SceneGame.h"
#include "Graphics.h"
#include "Camera.h"
#include "SceneManager.h"
#include "GpuResourceUtils.h"

#include <imgui.h>

// シェーダー
#include "SkyMap.h"
#include "ShadowMap.h"

// 初期化
void SceneGame::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAt(
		{ 0, 10, 10 },		// 視点
		{ 0, 0, 0 },		// 注視点
		{ 0, 1, 0 }			// 上ベクトル
	);

	// カメラコントローラー初期化
	cameraController = std::make_unique<FreeCameraController>();
	cameraController->SyncCameraToController(camera);

	// ステージ初期化
	stage = std::make_unique<Stage>();

	// プレイヤー初期化
	player = std::make_unique<Player>();

	// エネミー初期化
	enemy = std::make_unique<Enemy>();

	// フレームバッファ
	framebuffers[0] = std::make_unique<framebuffer>(device, screenWidth, screenHeight, true);
	bit_block_transfer = std::make_unique<fullscreen_quad>(device);

	// ブルーム関連
	bloomer = std::make_unique<bloom>(device, screenWidth, screenHeight);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/final_pass_ps.cso", pixel_shaders[0].ReleaseAndGetAddressOf());
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// カメラ更新処理
	cameraController->Update();
	cameraController->SyncControllerToCamera(camera);

	// ステージ更新
	stage->Update(elapsedTime);

	// プレイヤー更新
	player->Update(elapsedTime);

	// エネミー更新
	enemy->Update(elapsedTime);
}

// 描画処理
void SceneGame::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();

	// レンダーステート設定
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// シャドウマップ取得
	Graphics& graphics = Graphics::Instance();
	ShadowMap* shadowMap = graphics.GetShadowMap();


	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.pbrMetalness = this->pbrMetalness;
	rc.pbrRoughness = this->pbrRoughness;
	rc.lightManager = &lightManager;

	// シャドウマップ準備
	//RenderContextに対してシャドウマップの情報を格納する
	ShadowMapData shadowData;
	shadowData.shadowMap = shadowMap->GetShaderResourceView();
	shadowData.shadowSampler = shadowMap->GetSamplerState();
	shadowData.lightViewProjection = lightViewProjection;
	shadowData.shadowColor = DirectX::XMFLOAT4(0, 0, 0, 0.1);
	shadowData.shadowBias = shadowBias;
	shadowData.shadowAttenuation = shadowAttenuation;
	rc.shadowMapData = &shadowData;

	// シャドウマップ描画
	RenderShadowMap();

	// 3Dモデル描画
	{
		// ステージ描画
		stage->Render(rc, modelRenderer);

		//プレイヤー描画
		player->Render(rc, modelRenderer);

		// エネミー描画
		enemy->Render(rc, modelRenderer);

		framebuffers[0]->clear(dc);
		framebuffers[0]->activate(dc);

		modelRenderer->Render(rc);

		// 2D描画
		{
			modelRenderer->GetShader<SkyMap>(ShaderId::SkyMap)->Draw(rc);
		}

		framebuffers[0]->deactivate(dc);

		// ブルーム描画
		bloomer->make(dc, framebuffers[0]->shader_resource_views[0].Get(), renderState);

		ID3D11ShaderResourceView* shader_resource_views[] =
		{
			framebuffers[0]->shader_resource_views[0].Get(),
			bloomer->shader_resource_view(),
		};

		bloomer->set_constant_buffer(dc);
		bit_block_transfer->blit(dc, shader_resource_views, 0, 2, pixel_shaders[0].Get());
	}
}

// シャドウマップ描画
void SceneGame::RenderShadowMap()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	Graphics& graphics = Graphics::Instance();

	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// シャドウマップ取得
	ShadowMap* shadowMap = graphics.GetShadowMap();

	// レンダーステート設定
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;

	//１カメラをライトから見た情報に変更
	{
		// 最初のカメラの情報を保存する
		camera.SaveCamera();

		// ライト方向を取得
		DirectX::XMFLOAT3 lightDir = lightManager.GetDirectionalLight().direction;

		// ライトビュー行列を計算
		// ライト方向の反対側にライトを配置
		DirectX::XMFLOAT3 lightPos = { -lightDir.x * 50.0f, -lightDir.y * 50.0f, -lightDir.z * 50.0f };
		DirectX::XMFLOAT3 lightTarget = { 0, 0, 0 }; // 原点を向くようにする
		DirectX::XMFLOAT3 lightEyeUp = { 0, 1, 0 }; // 上方向を向く

		camera.SetLookAt(lightPos, lightTarget, lightEyeUp);

		// ライトプロジェクション行列を計算
		//※調節する時近平面と遠平面の差が大きいとジャギが出やすい。
		//※ジャギを抑えるには解像度を上げる・バイアスを小さくするなど
		camera.SetOrthographic(
			35.0,   // 横幅
			35.0,   // 高さ
			20.0f,    // 近平面
			100.0f // 遠平面
		);

		DirectX::XMFLOAT4X4 view = camera.GetView();
		DirectX::XMFLOAT4X4 projaection = camera.GetProjection();
		DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
		DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projaection);

		// ライトビュープロジェクション行列を計算して保存
		// シャドウマップ用の情報に入れる
		DirectX::XMMATRIX lightViewProjection_M = DirectX::XMMatrixMultiply(V, P);
		DirectX::XMStoreFloat4x4(&lightViewProjection, lightViewProjection_M);
	}

	shadowMap->Activate(dc);
	{
		// ステージ描画
		stage->Render(rc, modelRenderer);

		//プレイヤー描画
		player->Render(rc, modelRenderer);

		modelRenderer->Render(rc);
	}
	shadowMap->Deactivate(dc);

	//２変更したカメラの情報を元に戻すこと
	camera.ResetCamera();
}


// GUI描画
void SceneGame::DrawGUI()
{
	Graphics& graphics = Graphics::Instance();
	ShadowMap* shadowMap = graphics.GetShadowMap();

	ImGui::Begin("Shader");
	RenderContext rc;

	if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DirectionalLight& light = lightManager.GetDirectionalLight();
		// 並行光源
		ImGui::DragFloat3("light", &light.direction.x, 0.01f, -1.0, 1.0);
		ImGui::ColorEdit3("light color", &light.color.x);
		lightManager.SetDirectionalLight(light);
	}

	if (ImGui::CollapsingHeader("PBR", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Adjust Metalness", &pbrMetalness, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat("Adjust Roughness", &pbrRoughness, 0.01f, -1.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("ShadowMap", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderFloat("Shadow Attenuation", &shadowAttenuation, 0.0f, 1.0f);
		ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0f, 0.01f);

		ImGui::Text("texture");
		ImGui::Image(shadowMap->GetShaderResourceView(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });
	}

	if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("bloom_extraction_threshold", &bloom_extraction, 0.01f, +0.0f, +5.0f);
		ImGui::DragFloat("bloom_intensity", &bloom_intensity, 0.01f, +0.0f, +5.0f);
		ImGui::DragFloat("exposure", &bloomer->exposure, 0.01f, 0.0f, 1.0f);

		bloomer->bloom_extraction_threshold = bloom_extraction;
		bloomer->bloom_intensity = bloom_intensity;

		ImGui::Separator();

		// グロー抽出
		ImGui::Text("Glow Extraction");
		ImGui::Image((ImTextureID)bloomer->get_glkow_extraction(), ImVec2(256, 256));

		// ガウシアンブラー各段階
		for (size_t i = 0; i < bloom::downsampled_count; ++i)
		{
			ImGui::Text("Gaussian Blur [%zu][0]", i);
			ImGui::Image((ImTextureID)bloomer->get_gaussian_blur(i, 0), ImVec2(256, 256));
			ImGui::Text("Gaussian Blur [%zu][1]", i);
			ImGui::Image((ImTextureID)bloomer->get_gaussian_blur(i, 1), ImVec2(256, 256));
		}
	}

	ImGui::End();


	player->DrawGui();
	enemy->DrawGui();
}