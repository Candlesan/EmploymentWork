#include "SceneBuild.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲーム
#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Load/SceneLoad.h"
#include "GamePlay/Scene/scene/Game/SceneGame.h"

#include "GamePlay/Session/GameSession.h"
#include "GamePlay/Object/Magic/Loadout/MagicCatalog.h"

#include <imgui.h>


// 初期化
void SceneBuild::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneBuild.json");
	uiCanvas.SetPanel("Slot");
	RefreshEquippedMarks();
}

// 更新処理
void SceneBuild::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	UIElement* fade = uiCanvas.Find("fade");

	// UI更新
	uiEditor.Update(elapsedTime);
	uiCanvas.Update(elapsedTime);
	uiCanvas.UpdateNavigation(gamePad);

	// Aボタンで装備する
	if (gamePad.GetButtonDown() & GamePad::BTN_A)
	{
		int slot = uiCanvas.GetFocusIndex(); // タブの番号
		int magic = uiCanvas.GetSubFocusIndex(); // タブの中の番号

		if (slot >= 0 && magic >= 0 && magic < MagicCatalog::GetCount(static_cast<MagicSlotIndex>(slot))) // 表にある番号だけ
		{
			GameSession::Instance().equippedIndex[slot] = magic;
			RefreshEquippedMarks(); // 印を付け直す
		}
	}

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

	UIElement* t = uiCanvas.GetFocused();
	UIElement* m = uiCanvas.GetSubFocused();
	ImGui::Text(u8"タブ: %s / 魔法: %s", t ? t->name.c_str() : u8"なし", m ? m->name.c_str() : u8"なし");
}

// 画面比率修正関数
void SceneBuild::OnResize(float width, float height)
{
	// カメラの比率を直す
	Camera& camera = CameraManager::Instance().GetMainCamera();
	camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45), width / height, 0.1f, 1000.0f);

	ID3D11Device* device = Graphics::Instance().GetDevice();
}

// 装備中の魔法に Equipped の印を付ける
void SceneBuild::RefreshEquippedMarks()
{
	static const char* tabNames[MAGIC_SLOT_COUNT] = { "List_RB", "List_RT", "List_LB", "List_LT" };

	for (int slot = 0; slot < MAGIC_SLOT_COUNT; ++slot)
	{
		UIElement* tab = uiCanvas.Find(tabNames[slot]);
		if (!tab) continue;

		int equipped = GameSession::Instance().equippedIndex[slot];
		for (auto& button : tab->children)
		{
			if (button->focusIndex < 0) continue;              // 魔法ボタン以外（タブの Highlight）は飛ばす
			UIElement* mark = uiCanvas.FindElement(button.get(), "Equipped");
			if (mark) mark->visible = (button->focusIndex == equipped);
		}
	}
}
