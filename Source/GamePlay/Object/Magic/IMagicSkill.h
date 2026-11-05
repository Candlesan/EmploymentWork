#pragma once
#include "GamePlay/Object/Magic/MagicContext.h"

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
	virtual const char* GetName() const = 0;
};