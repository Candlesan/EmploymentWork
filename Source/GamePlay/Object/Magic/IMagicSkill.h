#pragma once
#include "GamePlay/Object/Magic/MagicContext.h"
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"

// ‰Ÿ‚³‚ê‚½‚ç‰½‚ğ‚·‚é‚©
class IMagicSkill
{
public:
	virtual ~IMagicSkill() = default;
	virtual void OnPress(const MagicContext& ctx) {} // ‰Ÿ‚³‚ê‚½uŠÔ
	virtual void OnHold(const MagicContext& ctx, float elapsedTime){} // ’·‰Ÿ‚µ
	virtual void OnRelease(const MagicContext& ctx){} // —£‚µ‚½‚Æ‚«
	virtual void Update(const MagicContext& ctx, float elapsedTime) {}
	virtual void DrawGUI() {}
	virtual void RenderDebugPrimitive(const MagicContext& ctx, ShapeRenderer* renderer) {}
	virtual const char* GetName() const = 0;
	virtual bool IsBusy() const { return false; }
	virtual bool IsReady() const { return true; } // ¡g‚¦‚éó‘Ô‚©
};