#pragma once

#include "GamePlay/Object/Camera/Camera.h"
#include "System/Graphic/RenderState.h"
#include "System/Renderer/Light.h"

struct RenderContext
{
	ID3D11DeviceContext*	deviceContext;
	const RenderState*		renderState;
	const Camera*			camera;
	const LightManager*		lightManager = nullptr;

	// PBR—p
	float pbrMetalness = 0.0f;
	float pbrRoughness = 0.0f;
};
