#pragma once
#include "System/Graphic/RenderContext.h"
#include "System/Renderer/Light/Light.h"

#include <memory>
#include <functional>
#include <DirectXMath.h>

class SkyMap;
class fullscreen_quad;
class framebuffer;
class bloom;

class SceneRenderEnv
{
public:
	SceneRenderEnv();
	~SceneRenderEnv();

	void Initialize();
	void Render(const RenderContext& rc);

	void RenderStageShadow(const DirectX::XMFLOAT3& volumeMin,
						   const DirectX::XMFLOAT3& volumeMax,
		                   const std::function<void(const RenderContext&)>& drawCasters);
	void DrawGUI();

	void OnResize(float w, float h);
	void InvalidateShadow() { stageShadowRendered = false; }

	void BeginScene(const RenderContext& rc);
	void EndScene(const RenderContext& rc);
	void Present(const RenderContext& rc);

	void SetupRenderContext(RenderContext& rc);
	LightManager& GetLightManager() { return lightManager; }
	float GetStageRadius() const { return stageRadius; }
private:
	DirectX::XMFLOAT3 CalcLightDirection() const;

private:
	std::unique_ptr<SkyMap> skyMap; // スカイマップ

	// PBR調整用
	float pbrMetalness = 0.0f;
	float pbrRoughness = 0.0f;
	float ambientIntensity = 0.01f;

	// ライト
	LightManager lightManager;
	float lightYaw = 36.0f;   // 方位角（度）  0〜360
	float lightPitch = 40.0;  // 仰角（度）   0=地平線, 90=真上

	// シャドウマップ
	ShadowMapData stageShadowData;
	float stageShadowAttenuation = 0.0f;
	float stageShadowBias = 0.006f;
	DirectX::XMFLOAT4X4 stageLightViewProjection{};
	float stageRadius = 0.0f;
	bool stageShadowRendered = false;

	// ポストエフェクト
	std::unique_ptr<framebuffer> framebuffers[3];
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shaders[3];
	std::unique_ptr<fullscreen_quad> bit_block_transfer;

	// ブルーム
	std::unique_ptr<bloom> bloomer;
	float bloom_extraction = 1.87f;
	float bloom_intensity = 1.79f;
	float bloom_exposure = 0.45;
};