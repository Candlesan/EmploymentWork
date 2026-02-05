#include "SceneGame.h"
#include "Graphics.h"
#include "Camera.h"
#include "SceneManager.h"

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
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// カメラ更新処理
	cameraController->Update();
	cameraController->SyncControllerToCamera(camera);

	stage->Update(elapsedTime);
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

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;

	// 3Dモデル描画
	{
		// ステージ描画
		stage->Render(rc, modelRenderer);
		modelRenderer->Render(rc);
	}
}

// GUI描画
void SceneGame::DrawGUI()
{
}