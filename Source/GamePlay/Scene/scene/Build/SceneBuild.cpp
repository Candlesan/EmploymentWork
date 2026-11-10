#include "SceneBuild.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲーム
#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Load/SceneLoad.h"
#include "GamePlay/Scene/scene/Game/SceneGame.h"

#include <imgui.h>


// 初期化
void SceneBuild::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneBuild.json");
}

// 更新処理
void SceneBuild::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	UIElement* fade = uiCanvas.Find("fade");

	// UI更新
	uiEditor.Update(elapsedTime);
	uiCanvas.Update(elapsedTime);

	// フェード中
	if (isFadeing)
	{
		if (!uiCanvas.IsPlaying(fade))
		{
			SceneManager::Instance().ChangeScene(new SceneLoad(new SceneGame));
		}
		return; // フェード中は操作を受け付けない
	}

	// Xボタンを押してフェード開始
	if (gamePad.GetButtonDown() & GamePad::BTN_X)
	{
		uiCanvas.Play(fade);
		isFadeing = true;
	}
}

// 描画処理
void SceneBuild::Render()
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

	// 2Dスプライト描画
	{
		uiEditor.Render(dc);
		uiCanvas.Render(dc);
	}
}

// GUI描画
void SceneBuild::DrawGUI()
{
	uiEditor.DrawGUI();
}

// 画面比率修正関数
void SceneBuild::OnResize(float width, float height)
{
	// カメラの比率を直す
	Camera& camera = CameraManager::Instance().GetMainCamera();
	camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45), width / height, 0.1f, 1000.0f);

	ID3D11Device* device = Graphics::Instance().GetDevice();
}