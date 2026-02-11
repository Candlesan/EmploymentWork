#include "SceneGame.h"
#include "System/Graphic/Graphics.h"
#include "Gameplay/Object/Camera/Camera.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "SceneManager.h"

#include <imgui.h>

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

	// ゲーム用のカメラを初期化
	gameCamera = std::make_unique<GameCamera>();

	// デバック用のカメラを初期化
	debugCamera = std::make_unique<DebugCamera>();
	debugCamera->SyncCameraToController(camera);

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

	// GUIの操作でゲーム用とデバック用のカメラを切り替え
	if (cameraMode == CameraMode::Game)
	{
		// ゲーム用のカメラ更新
		DirectX::XMFLOAT3 target = player->GetPosition();
		target.y += 1.0f; // 注視点を少し上にあげる
		gameCamera->SetTarget(target);
		gameCamera->Update(elapsedTime);
	}
	else
	{
		// デバック用のカメラ更新
		debugCamera->Update();
		debugCamera->SyncControllerToCamera(camera);
	}

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
	ImGui::Begin("Camera System");

	// モード切り替え用のラジオボタン
	int mode = static_cast<int>(cameraMode);
	if (ImGui::RadioButton("Game (3rd Person)", &mode, 0)) {
		cameraMode = CameraMode::Game;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Debug (Free)", &mode, 1)) {
		cameraMode = CameraMode::Debug;
		// 切り替えた瞬間に現在のカメラ位置をデバッグ用カメラに同期させる
		debugCamera->SyncCameraToController(CameraManager::Instance().GetMainCamera());
	}

	ImGui::End();
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
