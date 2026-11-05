#include "MagicLoadout.h"

// XVˆ—
void MagicLoadout::Update(const MagicContext& ctx, const MagicCommand& cmd, float elapsedTime)
{
	for (int i = 0; i < 4; ++i)
	{
		IMagicSkill* skill = slots[i].GetEquipped(); // ‘•”õ‚ğæ“¾‚·‚é
		if (!skill) continue;

		if (cmd.press[i]) skill->OnPress(ctx);
		if (cmd.hold[i]) skill->OnHold(ctx, elapsedTime);
		if (cmd.release[i]) skill->OnRelease(ctx);
		skill->Update(ctx, elapsedTime);
	}
}