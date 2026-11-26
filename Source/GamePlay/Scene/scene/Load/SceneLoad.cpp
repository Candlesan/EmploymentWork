#include "SceneLoad.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"
#include "System/UI/UIEditor/UIEditor.h"
#include "System/UI/UIEditor/UICanvas.h"

// ゲーム
#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/scene/Game/SceneGame.h"

#include <imgui.h>


// 初期化
void SceneLoad::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);

	// UIをロード
	uiCanvas.Load("Data/Json/UI/SceneLoad.json");

	//スレッド開始
	thread = new std::thread(LoadingThread, this);
}

// 終了化
void SceneLoad::Finalize()
{
	//スレッド終了化
	if (thread != nullptr)
	{
		thread->join();
		delete thread;
		thread = nullptr;
	}
	if (nextScene != nullptr) 
	{
		nextScene->Finalize();
		delete nextScene;
		nextScene = nullptr;
	}
}

// 更新処理
void SceneLoad::Update(float elapsedTime)
{
	uiEditor.Update(elapsedTime);
	uiCanvas.Update(elapsedTime);

	uiCanvas.Play(uiCanvas.Find("NowLoading"));

	//次のシーンの準備が完了したらシーンを切り替える
	if (nextScene->IsReady())
	{
		SceneManager::Instance().ChangeScene(nextScene);
		nextScene = nullptr;
	}
}

// 描画処理
void SceneLoad::Render()
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
void SceneLoad::DrawGUI()
{
	uiEditor.DrawGUI();
}

//ローディングスレッド
void SceneLoad::LoadingThread(SceneLoad* scene)
{
	//COM関連の初期化でスレッド毎に呼ぶ必要がある
	CoInitialize(nullptr);

	//次のシーンの初期化を行う
	scene->nextScene->Initialize();

	//スレッドが終わる前にCOM関連の終了化
	CoUninitialize();

	//次のシーンの準備完了設定
	scene->nextScene->SetReady();
}
