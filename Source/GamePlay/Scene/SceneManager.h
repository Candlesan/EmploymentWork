#pragma once

#include "Scene.h"

//シーンマネージャー
class SceneManager
{
private:
	SceneManager() {}
	~SceneManager() {}

public:
	//唯一のインスタンスを取得
	static SceneManager& Instance()
	{
		static SceneManager instance;
		return instance;
	}

	//更新処理
	void Update(float elpasedTime);

	//描画処理
	void Render();

	//GUI描画
	void DrawGUI();

	//シーンクリア
	void Clear();

	//シーン切り替え
	void ChangeScene(Scene* scene);
private:
	Scene* currenScene = nullptr;
	Scene* nextScene = nullptr;
};
