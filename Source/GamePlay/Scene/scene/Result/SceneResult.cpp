#include "SceneResult.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲーム
#include "GamePlay/Session/GameSession.h"

#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Load/SceneLoad.h"
#include "GamePlay/Scene/scene/Title/SceneTitle.h"
#include "GamePlay/Scene/scene/Game/SceneGame.h"
#include "GamePlay/Scene/scene/Build/SceneBuild.h"

#include "Gameplay/Object/Camera/Camera.h"


#include <imgui.h>


void SceneResult::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneResult.json");
	uiCanvas.SetPanel("Slot");

	// クリアタイムを計算して表示する
	int total = static_cast<int>(GameSession::Instance().clearTime);
	if (total > 99 * 60 + 59) total = 99 * 60 + 59;   // 99:59 で止める

	int minutes = total / 60;
	int seconds = total % 60;

	SetDigit("digit_0", minutes / 10);
	SetDigit("digit_1", minutes % 10);
	SetDigit("digit_2", seconds / 10);
	SetDigit("digit_3", seconds % 10);

	// 装備構成を表示する
	ShowEquipped("slot_RB", MagicSlotIndex::RB);
	ShowEquipped("slot_RT", MagicSlotIndex::RT);
	ShowEquipped("slot_LB", MagicSlotIndex::LB);
	ShowEquipped("slot_LT", MagicSlotIndex::LT);
}

void SceneResult::Finalize()
{
}

void SceneResult::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	UIElement* fade = uiCanvas.Find("fade");

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
			case SceneResult::ResultMenu::Retry:
				// シーンゲーム画面に遷移する
				GameSession::Instance().ResetRun();
				SceneManager::Instance().ChangeScene(new SceneLoad(new SceneGame));

				break;
			case SceneResult::ResultMenu::ReBuid:
				// ビルド画面に遷移する
				GameSession::Instance().ResetAll();
				SceneManager::Instance().ChangeScene(new SceneLoad(new SceneBuild));

				break;
			case SceneResult::ResultMenu::toTitle:
				// タイトル画面に遷移する
				SceneManager::Instance().ChangeScene(new SceneLoad(new SceneTitle));

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
		case SceneResult::ResultMenu::Retry:
		case SceneResult::ResultMenu::ReBuid:
		case SceneResult::ResultMenu::toTitle:
			uiCanvas.Play(fade); // フェードを再生する
			isFadeing = true;

			break;

		}
	}
}

void SceneResult::Render()
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

void SceneResult::DrawGUI()
{
	uiEditor.DrawGUI();
}

// 1文字を切り抜いて表示する
void SceneResult::SetDigit(const char* name, int number)
{
	UIElement* e = uiCanvas.Find(name);
	if (!e) return; // なければ何もしない

	DirectX::XMFLOAT2 tex = e->sprite->GetTextureSize();
	const float w = tex.x / 10.0f; // 1つの文字幅
	const float h = tex.y; // 1つの横幅
	e->uvRect = { number * w, 0.0f, 512, 512 };
}

// クリア装備を表示する
void SceneResult::ShowEquipped(const char* slotName, MagicSlotIndex slot)
{
	UIElement* parent = uiCanvas.Find(slotName);
	if (!parent) return;

	int equipped = GameSession::Instance().equippedIndex[static_cast<int>(slot)];

	for (auto& child : parent->children)
	{
		if (child->focusIndex < 0) continue;
		child->visible = (child->focusIndex == equipped);
	}
}