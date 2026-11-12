#include "SceneManager.h"

//更新処理
void SceneManager::Update(float elapsedTime)
{
	if (nextScene != nullptr)
	{
		//新しいシーンを設定
		Scene* next = nextScene;
		nextScene = nullptr;

		//古いシーンを終了処理
		Clear();

		currenScene = next;

		//シーンを初期化
		if (!currenScene->IsReady())
		{
			currenScene->Initialize();
		}
	}

	if (currenScene != nullptr)
	{
		currenScene->Update(elapsedTime);
	}
}

//描画処理
void SceneManager::Render()
{
	if (currenScene != nullptr)
	{
		currenScene->Render();
	}
}

//GUI描画
void SceneManager::DrawGUI()
{
	if (currenScene != nullptr)
	{
		currenScene->DrawGUI();
	}
}

//シーンクリア
void SceneManager::Clear()
{
	if (currenScene != nullptr)
	{
		currenScene->Finalize();
		delete currenScene;
		currenScene = nullptr;
	}
	if (nextScene != nullptr)
	{
		if (nextScene->IsReady()) nextScene->Finalize();   // 初期化済みのときだけ片付ける
		delete nextScene;
		nextScene = nullptr;
	}
}

//シーン切り替え
void SceneManager::ChangeScene(Scene* scene)
{
	//新しいシーンを設定
	nextScene = scene;
}
