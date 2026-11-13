#include "PauseMenu.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲーム
#include "Gameplay/Object/Camera/Camera.h"

#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Load/SceneLoad.h"
#include "GamePlay/Scene/scene/Title/SceneTitle.h"

#include <imgui.h>


void PauseMenu::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/PauseMenu.json");
	uiCanvas.SetPanel("Slot");
}

PauseMenu::Result PauseMenu::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// UIを更新する
	uiCanvas.Update(elapsedTime);
	uiCanvas.UpdateNavigation(gamePad);

	// Aボタンで決定する
	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		int index = uiCanvas.GetFocusIndex(); // スロット番号

		if (index >= 0 && index < 3) return items[index];
	}
	if (gamePad.GetButtonDown() & GamePad::BTN_B) return Result::Resume;

	return Result::None;
}

void PauseMenu::Render()
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
		uiCanvas.Render(dc);
	}
}