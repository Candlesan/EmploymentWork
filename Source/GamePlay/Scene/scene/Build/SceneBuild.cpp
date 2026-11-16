#include "SceneBuild.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲーム
#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Load/SceneLoad.h"
#include "GamePlay/Scene/scene/Game/SceneGame.h"
#include "GamePlay/Scene/scene/Title/SceneTitle.h"

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
	int slot = uiCanvas.GetFocusIndex(); // タブの番号
	int magic = uiCanvas.GetSubFocusIndex(); // タブの中の番号
	const int START_SLOT = 4;   // 「ゲームへ」ボタンの選択番号

	if (slot >= 0 && slot < MAGIC_SLOT_COUNT)
	{
		lastMagicSlot = slot;   // 魔法のタブにいるときは覚えておく
	}
	else if (slot == START_SLOT)
	{
		ShowMagicList(lastMagicSlot);   // スタートにいるときは、最後のタブの一覧を出す
	}

	// Aボタンで装備する
	if (gamePad.GetButtonDown() & GamePad::BTN_A && slot < MAGIC_SLOT_COUNT)
	{

		if (magic >= 0 && magic < MagicCatalog::GetCount(static_cast<MagicSlotIndex>(slot))) // 表にある番号だけ
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
			if (!backTitle)
			{
				SceneManager::Instance().ChangeScene(new SceneLoad(new SceneGame));
			}
			else
			{
				SceneManager::Instance().ChangeScene(new SceneLoad(new SceneTitle));
			}
		}
		return; // フェード中は操作を受け付けない
	}

	if ((gamePad.GetButtonDown() & GamePad::BTN_B))
	{
		uiCanvas.Play(fade);
		isFadeing = true;
		backTitle = true;
	}


	// Xボタンを押してフェード開始
	if ((gamePad.GetButtonDown() & GamePad::BTN_X) 
		|| (gamePad.GetButtonDown() & GamePad::BTN_A) && slot == START_SLOT)
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

// 指定したタブの魔法を表示する
void SceneBuild::ShowMagicList(int slotIndex)
{
	UIElement* e = uiCanvas.Find("Slot");
	if (!e) return;

	for (auto& tab : e->children)
	{
		if (tab->focusIndex != slotIndex) continue;

		for (auto& item : tab->children)
		{
			if (item->focusIndex >= 0) item->visible = true;
		}
	}
}
