#pragma once
#include "GamePlay/Object/Camera/Camera.h"
#include <DirectXMath.h>


//シーン
class Scene
{
public:
	Scene() {}
	virtual ~Scene() {}

	//初期化
	virtual void Initialize() = 0;

	//終了化
	virtual void Finalize() = 0;

	//更新処理
	virtual void Update(float elapsedTime) = 0;

	//描画処理
	virtual void Render() = 0;

	//GUI描画
	virtual void DrawGUI() = 0;

	//準備完了しているか
	bool IsReady() const { return ready; }

	//準備完了設定
	void SetReady() { ready = true; }

	void Resize(float w, float h)
	{
		if (w <= 0.0f || h <= 0.0f) return;
		Camera& camera = CameraManager::Instance().GetMainCamera();
		camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45.0f), w / h, 0.1f, 1000.0f);
		OnResize(w, h);
	}
protected:
	virtual void OnResize(float width, float height) {}

private:
	bool ready = false;
};