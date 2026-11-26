#include "SceneTitle.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

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
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneTitle.json");
	uiCanvas.SetPanel("Slot");
	
	// ゲームを初期状態に戻す
	GameSession::Instance().ResetAll();
}

// 更新処理
void SceneTitle::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

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
void SceneTitle::DrawGUI()
{
	uiEditor.DrawGUI();
}