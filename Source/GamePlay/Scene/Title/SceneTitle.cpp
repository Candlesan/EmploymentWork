#include "SceneTitle.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲーム
#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/Load/SceneLoad.h"
#include "GamePlay/Scene/Game/SceneGame.h"
#include "Gameplay/Object/Camera/Camera.h"

#include <imgui.h>


// 初期化
void SceneTitle::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneTitle.json");
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		SceneManager::Instance().ChangeScene(new SceneLoad(new SceneGame));
	}

	uiEditor.Update(elapsedTime);
	uiCanvas.Update(elapsedTime);
}

// 描画処理
void SceneTitle::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

	// レンダーステート設定
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	//rc.camera = &camera;
	//rc.pbrMetalness = pbrMetalness;
	//rc.pbrRoughness = pbrRoughness;
	//rc.lightManager = &lightManager;

	// 2Dスプライト描画
	{
		uiEditor.Render(dc);
		uiCanvas.Render(dc);
	}
}

// GUI描画
void SceneTitle::DrawGUI()
{
	uiEditor.DrawGUI();
}

// 画面比率修正関数
void SceneTitle::OnResize(float width, float height)
{
	// カメラの比率を直す
	Camera& camera = CameraManager::Instance().GetMainCamera();
	camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45), width / height, 0.1f, 1000.0f);

	ID3D11Device* device = Graphics::Instance().GetDevice();

	//framebuffers[0] = std::make_unique<framebuffer>(device, width, height, true);

	//// ブルームを新しいサイズで丸ごと生成し直す
	//bloomer = std::make_unique<bloom>(device, width, height);

	//// 初期化時に設定していたパラメータを再設定
	//bloomer->bloom_intensity = bloom_intensity;
	//bloomer->bloom_extraction_threshold = bloom_extraction;
}
