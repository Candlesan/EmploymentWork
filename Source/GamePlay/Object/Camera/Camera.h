#pragma once

#include <DirectXMath.h>

// カメラ
class Camera
{
public:
	Camera();

	// 指定方向を向く
	void SetLookAt(const DirectX::XMFLOAT3& eye, const DirectX::XMFLOAT3& focus, const DirectX::XMFLOAT3& up);

	// パースペクティブ設定
	void SetPerspectiveFov(float fovY, float aspect, float nearZ, float farZ);

	//正射影設定(シャドウマップに必要)
	void SetOrthographic(float width, float height, float nearZ, float farZ);

	// ビュー行列取得
	const DirectX::XMFLOAT4X4& GetView() const { return view; }

	// プロジェクション行列取得
	const DirectX::XMFLOAT4X4& GetProjection() const { return projection; }

	// 視点取得
	const DirectX::XMFLOAT3& GetEye() const { return eye; }

	// 注視点取得
	const DirectX::XMFLOAT3& GetFocus() const { return focus; }

	// 上方向取得
	const DirectX::XMFLOAT3& GetUp() const { return up; }

	// 前方向取得
	const DirectX::XMFLOAT3& GetFront() const { return front; }

	// 右方向取得
	const DirectX::XMFLOAT3& GetRight() const { return right; }

	// シャドウマップ用
	// 現在のカメラの状態を保存
	void SaveCamera();

	// カメラの状態を元に戻す
	void ResetCamera();

	// 視野角取得
	const float& GetFovY() { return fovY; }

	// アスペクト比取得
	const float& GetAspect() { return aspect; }

	// 近クリップ面までの距離を取得
	const float& GetNear() { return nearZ; }

	// 遠クリップ面までの距離を取得
	const float& GetFar() { return farZ; }

private:
	DirectX::XMFLOAT4X4		view;
	DirectX::XMFLOAT4X4		projection;

	DirectX::XMFLOAT3		eye;
	DirectX::XMFLOAT3		focus;

	DirectX::XMFLOAT3		up;
	DirectX::XMFLOAT3		front;
	DirectX::XMFLOAT3		right;

	float fovY;
	float aspect;
	float nearZ;
	float farZ;

	// カメラ情報を保存するための変数
	DirectX::XMFLOAT4X4 saveView;
	DirectX::XMFLOAT4X4 saveProjection;
	DirectX::XMFLOAT3 savedEye;
	DirectX::XMFLOAT3 savedFocus;
	DirectX::XMFLOAT3 savedUp;
	DirectX::XMFLOAT3 savedFront;
	DirectX::XMFLOAT3 savedRight;
	float savedNearZ;
	float savedFarZ;
	float savedFovY;
	float savedAspect;
};

// カメラマネージャー
class CameraManager
{
private:
	CameraManager() {}
	~CameraManager() {}

public:
	// インスタンス取得
	static CameraManager& Instance()
	{
		static CameraManager instance;
		return instance;
	}

	// メインカメラ取得
	Camera& GetMainCamera() { return mainCamera; }

private:
	Camera		mainCamera;
};
