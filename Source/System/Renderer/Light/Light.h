#pragma once
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"

#include <DirectXMath.h>
#include <vector>
#include <string>

// 平行光源
struct DirectionalLight
{
	DirectX::XMFLOAT3	direction = { 0, -1, 0 }; // 光の向き
	DirectX::XMFLOAT3	color = { 1, 1, 1 }; // 光の色
	float intensity = 1.0f; // 光の強さ
};

// ポイントライト
struct PointLight
{
	DirectX::XMFLOAT3 position = { 0, 0, 0 }; // 位置
	DirectX::XMFLOAT3	color = { 1, 1, 1 }; // 光の色
	float range = 1.0f; // 光の影響範囲
	float intensity = 1.0f; // 光の強さ
	bool enabled = true; // ライトで使用するか
};

// スポットライト
struct SpotLight
{
	DirectX::XMFLOAT3	position = { 0, 0, 0 };	  // 位置
	DirectX::XMFLOAT3	direction = { 0, -1, 0 }; // 光の向き
	DirectX::XMFLOAT3	color = { 1, 1, 1 };	  // 光の色
	float range = 5.0f;							  // 光の影響範囲
	float innerAngle = 15.0f;					  // 度。中心からここまでは全開
	float outerAngle = 25.0f;					  // 度。ここで0になる
	float intensity = 1.0f;						  // 光の強さ
	bool enabled = true;						  // ライトで使用するか
};

class LightManager
{
public:
	// シェーダー側の配列サイズと必ず合わせる
	static constexpr int MaxPointLights = 32;
	static constexpr int MaxSpotLights = 16;

	// Jsonの保存場所
	static constexpr const char* LightsFilePath = "Data/Json/Stage/Light/StageLight.json";
	void Save(const std::string& path) const;
	void Load(const std::string& path);

	// 毎フレーム作り直す一時ライト(分ける理由はライトのリストに入れると保存されてその位置に置きっぱになるから)
	void ClearFrameLights() { framePointLights.clear(); }
	void AddFramePointLight(const PointLight& light) { framePointLights.push_back(light); }
	const std::vector<PointLight>& GetFramePointLights() const { return framePointLights; }

	// 平行光源設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }

	// 平行光源・点光源・スポットライト
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }
	DirectionalLight& GetDirectionalLight() { return directionalLight; }
	std::vector<PointLight>& GetPointLights() { return pointLights; }
	const std::vector<PointLight>& GetPointLights() const { return pointLights; }
	std::vector<SpotLight>& GetSpotLights() { return spotLights; }
	const std::vector<SpotLight>& GetSpotLights() const { return spotLights; }

	void RenderDebugPrimitive(ShapeRenderer* renderer);
private:
	DirectionalLight	directionalLight;
	std::vector<PointLight> pointLights;
	std::vector<PointLight> framePointLights;
	std::vector<SpotLight> spotLights;
};
