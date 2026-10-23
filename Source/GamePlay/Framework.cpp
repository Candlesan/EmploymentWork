#include <memory>
#include <sstream>
#include <imgui.h>

#include "Framework.h"
#include "System/Graphic/Graphics.h"
#include "System/Renderer/ImGuiRenderer/ImGuiRenderer.h"
#include "System/Core/Input/Input.h"
#include "System/Effect/EffectManager.h"

// シーン
#include "Gameplay/Scene/Manager/SceneManager.h"
#include "GamePlay/Scene/Title/SceneTitle.h"
#include "GamePlay/Scene/Game/SceneGame.h"
#include "System/Audio/Audio.h"

// 垂直同期間隔設定
static const int syncInterval = 0;

// コンストラクタ
Framework::Framework(HWND hWnd)
	: hWnd(hWnd)
{
	// グラフィックス初期化
	Graphics::Instance().Initialize(hWnd);

	// IMGUI初期化
	ImGuiRenderer::Initialize(hWnd, Graphics::Instance().GetDevice(), Graphics::Instance().GetDeviceContext());

	// 入力初期化
	Input::Instance().Initialize(hWnd);

	// オーディオ初期化
	Audio::Instance().Initialize();

	// エフェクトマネージャー初期化 
	EffectManager::Instance().Initialize();

	// シーン初期化
	SceneManager::Instance().ChangeScene(new SceneGame);


	// 最初にボーダーレスのフルスクリーンを作る
	ToggleFullscreen();

	// グラフィック基盤をリサイズする
	Graphics::Instance().OnResize();

	float w = Graphics::Instance().GetScreenWidth();
	float h = Graphics::Instance().GetScreenHeight();

	Scene* currentScene = SceneManager::Instance().GetCurrentScene();
	if (currentScene != nullptr)
	{
		currentScene->OnResize(w, h);
	}

	srand(static_cast<unsigned int>(time(nullptr)));
}

// デストラクタ
Framework::~Framework()
{
	// IMGUI終了化
	ImGuiRenderer::Finalize();

	// シーン終了化
	SceneManager::Instance().Clear();

	// オーディオ終了化
	Audio::Instance().Finalize();

	// エフェクトマネージャー終了化 
	EffectManager::Instance().Finalize();
}

// 更新処理
void Framework::Update(float elapsedTime)
{
#ifdef _DEBUG
	bool isF1DownNow = (GetAsyncKeyState(VK_F1) & 0x8000);
	// GUIの表示/非表示切り替え
	if (isF1DownNow && !isF1PressedLastFrame)
	{
		GUIDrawSwitch = !GUIDrawSwitch;
	}
	isF1PressedLastFrame = isF1DownNow;
#endif

	// IMGUIフレーム開始処理	
	ImGuiRenderer::NewFrame();

	// 入力初期化
	Input::Instance().Update();

	// シーン更新処理
	SceneManager::Instance().Update(elapsedTime);
}

// 描画処理
void Framework::Render(float elapsedTime)
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// 別スレッド中にデバイスコンテキストが使われていた場合に 
	// 同時アクセスしないように排他制御する 
	std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());

	// 画面クリア
	Graphics::Instance().Clear(0, 0, 1.0f, 1);

	// レンダーターゲット設定
	Graphics::Instance().SetRenderTargets();

	// シーン描画処理
	SceneManager::Instance().Render();

#if _DEBUG
	// シーンGUI描画処理
	if (GUIDrawSwitch)
	{
		SceneManager::Instance().DrawGUI();


		// GUIでFPS表示
		ImGui::Begin("FPS");
		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("FPS : %.1f", io.Framerate);
		ImGui::Text("Frame : %.3f ms", 1000.0f / io.Framerate);
		ImGui::End();
	}

	// IMGUI描画
	ImGuiRenderer::Render(dc);
#endif

	// 画面表示
	Graphics::Instance().Present(syncInterval);
}

// フレームレート計算
void Framework::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.
	static int frames = 0;
	static float time_tlapsed = 0.0f;

	frames++;

	// Compute averages over one second period.
	if ((timer.TimeStamp() - time_tlapsed) >= 1.0f)
	{
		float fps = static_cast<float>(frames); // fps = frameCnt / 1
		float mspf = 1000.0f / fps;
		std::ostringstream outs;
		outs.precision(6);
		outs << "FPS : " << fps << " / " << "Frame Time : " << mspf << " (ms)";
		SetWindowTextA(hWnd, outs.str().c_str());

		// Reset for next average.
		frames = 0;
		time_tlapsed += 1.0f;
	}
}

// フルスクリーンに切り替える
void Framework::ToggleFullscreen()
{
	isFullscreen = !isFullscreen;

	IDXGISwapChain* swapchain = Graphics::Instance().GetSwapChain();

	if (isFullscreen)
	{
		// ウィンドウモード時のサイズを保存
		GetWindowRect(hWnd, &windowedRect);

		// モニターのサイズを取得
		HMONITOR monitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(monitor, &mi);
		int w = mi.rcMonitor.right - mi.rcMonitor.left;
		int h = mi.rcMonitor.bottom - mi.rcMonitor.top;

		// ボーダーレスウィンドウでモニターサイズに拡大
		SetWindowLong(hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(hWnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top, w, h,
			SWP_FRAMECHANGED);

		// GraphicsのOnResizeではなく、確定したw, hを直接シーンに伝える！
		Scene* currentScene = SceneManager::Instance().GetCurrentScene();
		if (currentScene != nullptr)
		{
			currentScene->OnResize(static_cast<float>(w), static_cast<float>(h));
		}
	}
	else
	{
		// 元のウィンドウスタイルとサイズに戻す
		SetWindowLong(hWnd, GWL_STYLE,
			WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME | WS_VISIBLE);

		int w = windowedRect.right - windowedRect.left;
		int h = windowedRect.bottom - windowedRect.top;

		SetWindowPos(hWnd, HWND_TOP,
			windowedRect.left, windowedRect.top, w, h,
			SWP_FRAMECHANGED);

		// ウィンドウモードに戻ったときも、元のサイズ（w, h）を直接伝える！
		Scene* currentScene = SceneManager::Instance().GetCurrentScene();
		if (currentScene != nullptr)
		{
			currentScene->OnResize(static_cast<float>(w), static_cast<float>(h));
		}
	}
}

// アプリケーションループ
int Framework::Run()
{
	MSG msg = {};

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			timer.Tick();
			CalculateFrameStats();

			float elapsedTime = timer.TimeInterval();
			Update(elapsedTime);
			Render(elapsedTime);
		}
	}
	return static_cast<int>(msg.wParam);
}

// メッセージハンドラ
LRESULT CALLBACK Framework::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGuiRenderer::HandleMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc;
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) PostMessage(hWnd, WM_CLOSE, 0, 0);
		if (wParam == VK_F11) ToggleFullscreen(); // F11を押すとフルスクリーンに切り替え
		break;
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			// グラフィック基盤をリサイズする
			Graphics::Instance().OnResize();

			float w = Graphics::Instance().GetScreenWidth();
			float h = Graphics::Instance().GetScreenHeight();

			Scene* currentScene = SceneManager::Instance().GetCurrentScene();
			if (currentScene != nullptr)
			{
				currentScene->OnResize(w, h);
			}
		}
		break;
	case WM_ENTERSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
		timer.Stop();
		break;
	case WM_EXITSIZEMOVE:
		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
		timer.Start();
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
