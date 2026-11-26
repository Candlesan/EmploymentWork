#include "SceneTitle.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"


// ゲーム
#include "Gameplay/Object/Camera/Camera.h"

#include "GamePlay/Session/GameSession.h"

#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Load/SceneLoad.h"
#include "GamePlay/Scene/scene/Build/SceneBuild.h"

#include <imgui.h>


// 初期化
void SceneTitle::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// 描画環境（空・ライト・影・ブルーム）
	sceneRender.Initialize();

	// ステージ
	stage = std::make_unique<Stage>();

	// カメラ
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1500.0f								// ファークリップ
	);

	camera.SetLookAt({ 3.37, 1.74, -0.37 }, { -0.19, 2.17, -0.91 }, { 0, 1, 0 });

	// デバック用のカメラを初期化
	debugCamera = std::make_unique<DebugCamera>();
	debugCamera->SyncCameraToController(camera);

	// 自作エフェクトマネージャー初期化
	CustomEffectManager::Instance().Initialize(device);

	// シングルトンなので、リトライ前のゲームで出ていたエフェクト（防御魔法など）が残らないように消す
	CustomEffectManager::Instance().StopAll();

	// ステージの常時エフェクト
	CustomEffectManager::Instance().LoadEffectConfig("StageMana", "Data/Json/Effect/StageMana.json");
	CustomEffectManager::Instance().LoadEffectConfig("MoonStone_Light", "Data/Json/Effect/MoonStone_Light.json");
	CustomEffectManager::Instance().LoadEffectConfig("Star", "Data/Json/Effect/Star.json");
	CustomEffectManager::Instance().PlayEffect(
		"StageMana", { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	CustomEffectManager::Instance().PlayEffect(
		"MoonStone_Light", { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });
	CustomEffectManager::Instance().PlayEffect(
		"Star", { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f });

	// UIエディターの初期化
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneTitle.json");
	uiCanvas.SetPanel("Slot");
	
	// ゲームを初期状態に戻す
	GameSession::Instance().ResetAll();
}

// 終了化
void SceneTitle::Finalize()
{
	CustomEffectManager::Instance().StopAll();
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// 3D の更新
	stage->Update(elapsedTime);

	LightManager& lightManager = sceneRender.GetLightManager();
	lightManager.ClearFrameLights();
	CustomEffectManager::Instance().Update(elapsedTime);
	CustomEffectManager::Instance().CollectLights(lightManager);

	// デバック用のカメラ更新
	debugCamera->Update();
	debugCamera->SyncControllerToCamera(camera);

	UIElement* fade = uiCanvas.Find("fade");

	// UIを更新する
	uiEditor.Update(elapsedTime);
	uiCanvas.Update(elapsedTime);
	uiCanvas.UpdateNavigation(gamePad);

	// フェード中
	if (isFadeing)
	{
		if (!uiCanvas.IsPlaying(fade))
		{
			switch (selected)
			{
			case SceneTitle::GameMenu::GameStart:
				// ビルド画面に遷移する
				SceneManager::Instance().ChangeScene(new SceneLoad(new SceneBuild));

				break;
			case SceneTitle::GameMenu::Config:
				break;
			case SceneTitle::GameMenu::GameEnd:
				PostQuitMessage(0); // ゲームを終了する
				break;
			}
		}
		return; // フェード中は操作を受け付けない
	}

	// Aボタンを押してフェード開始
	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		int index = uiCanvas.GetFocusIndex(); // スロット番号を取得
		if (index < 0 || index >= 3) return;

		selected = items[index];

		switch (selected)
		{
		case SceneTitle::GameMenu::GameStart:
		case SceneTitle::GameMenu::GameEnd:
			uiCanvas.Play(fade); // フェードを再生する
			isFadeing = true;

			break;

		case SceneTitle::GameMenu::Config:
			break;
		}
	}
}

// 描画処理
void SceneTitle::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// レンダーステート設定
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// ステージの影
	sceneRender.RenderStageShadow(stage->GetVolumeMin(), stage->GetVolumeMax(),
		[&](const RenderContext& src) {
			stage->Render(src, modelRenderer);
			modelRenderer->Render(src);
		});

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	sceneRender.SetupRenderContext(rc);
	rc.isShadowPass = false;

	sceneRender.BeginScene(rc);
	{
		stage->Render(rc, modelRenderer);
		modelRenderer->Render(rc);
		sceneRender.SkyRender(rc);                         // 空
		CustomEffectManager::Instance().Render(rc);
		CustomEffectManager::Instance().RenderMeshVFX(modelRenderer);
	}
	sceneRender.EndScene(rc);
	sceneRender.Present(rc);

	// 2Dスプライト描画
	{
		uiEditor.Render(dc);
		uiCanvas.Render(dc);
	}
}

// GUI描画
void SceneTitle::DrawGUI()
{
	Camera& camera = CameraManager::Instance().GetMainCamera();

	uiEditor.DrawGUI();

	ImGui::Begin(u8"カメラ");
	const DirectX::XMFLOAT3& eye = camera.GetEye();
	const DirectX::XMFLOAT3& focus = camera.GetFocus();
	ImGui::Text(u8"視点  : %.2f, %.2f, %.2f", eye.x, eye.y, eye.z);
	ImGui::Text(u8"注視点: %.2f, %.2f, %.2f", focus.x, focus.y, focus.z);
	ImGui::End();


	if (ImGui::Begin(u8"タイトル描画設定"))
	{
		sceneRender.DrawGUI();
	}
	ImGui::End();
}

void SceneTitle::OnResize(float w, float h)
{
	sceneRender.OnResize(w, h);
}