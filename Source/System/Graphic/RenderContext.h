#pragma once

#include "GamePlay/Object/Camera/Camera.h"
#include "System/Graphic/RenderState.h"
#include "System/Renderer/Light/Light.h"

//	シャドウマップ用情報
struct ShadowMapData
{
	ID3D11ShaderResourceView*	shadowMap;				//	シャドウマップテクスチャ
	ID3D11SamplerState*			shadowSampler;			// サンプラーステート
	DirectX::XMFLOAT4X4			lightViewProjection;	//	ライトビュープロジェクション行列
	DirectX::XMFLOAT4			shadowColor;			//	影の色
	float						shadowBias;				//	深度比較用のオフセット値
	float						shadowAttenuation;		//  影の減衰のオフセット値
};

struct RenderContext
{
	ID3D11DeviceContext*	deviceContext;
	const RenderState*		renderState;
	const Camera*			camera;
	const LightManager*		lightManager = nullptr;

	const ShadowMapData* stageShadowMapData = nullptr;
	const ShadowMapData* characterShadowMapData = nullptr;
	bool isShadowPass = false;

	// PBR用
	float pbrMetalness = 0.0f;
	float pbrRoughness = 0.0f;

	// IBL用
	float ambientIntensity = 1.0f; // 環境光の強さ
};
