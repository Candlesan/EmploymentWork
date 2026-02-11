#include "SceneGame.h"
#include "System/Graphic/Graphics.h"
#include "Gameplay/Object/Camera/Camera.h"
#include "GamePlay/Object/Collision/Collision.h"
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

	// プレイヤー初期化
	player = std::make_unique<Player>();
	player->Initialize();

	// エネミー初期化
	enemy = std::make_unique<Enemy>();
	enemy->Initialize();
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

	// プレイヤーと敵の当たり判定
	CollisonPlayervsEnemy();

	// プレイヤーの武器と敵の当たり判定
	CollisionPlayerWeaponVsEnemy();
}

// 描画処理
void SceneGame::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

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

		// プレイヤー描画
		player->Render(rc, modelRenderer);

		// エネミー描画
		enemy->Render(rc, modelRenderer);

		// 全モデル描画
		modelRenderer->Render(rc);
	}

	// デバック描画
	{
		player->RenderDebugPrimitive(shapeRenderer);
		enemy->RenderDebugPrimitive(shapeRenderer);

		// シェイプレンダラ描画
		shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection());
	}
}

// GUI描画
void SceneGame::DrawGUI()
{
	player->DrawGUI();
	enemy->DrawGUI();
}

// プレイヤーと敵の当たり判定
void SceneGame::CollisonPlayervsEnemy()
{
	DirectX::XMFLOAT3 outPositionA, outPositionB;
	if (Collision::IntersectCapsuleVsCapsule(
		player->GetPosition(),
		player->GetCapsuleDirection(),
		player->GetHeight(),
		player->GetRadius(),
		player->GetWeight(),
		enemy->GetPosition(),
		enemy->GetCapsuleDirection(),
		enemy->GetHeight(),
		enemy->GetRadius(),
		enemy->GetWeight(),
		outPositionA,
		outPositionB
	))
	{
		// 押し出し処理
		player->SetPosition(outPositionA);
		enemy->SetPosition(outPositionB);
	}
}

// 武器と敵の当たり判定
void SceneGame::CollisionPlayerWeaponVsEnemy()
{
	DirectX::XMFLOAT3 outPositionA, outPositionB;
	if (Collision::IntersectCapsuleVsCapsule(
		player->GetWeaponPosition(),
		player->GetWeaponDirection(),
		player->GetWeaponHeight(),
		player->GetWeaponRadius(),
		1.0f, // 武器の重さ(適当に設定)
		enemy->GetPosition(),
		enemy->GetCapsuleDirection(),
		enemy->GetHeight(),
		enemy->GetRadius(),
		enemy->GetWeight(),
		outPositionA,
		outPositionB
	))
	{
		// 押し出し処理
		enemy->ApplyDamage(1.0f, 0.5f);
	}
}
