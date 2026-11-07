#include "MagicSlot.h"


// Œó•â‚ğ‘«‚·ŠÖ”
void MagicSlot::AddCandidate(std::unique_ptr<IMagicSkill> skill)
{
	candidates.push_back(std::move(skill));
}

// ‘•”õ’†‚Ì‹Z‚ğ•Ô‚·ŠÖ”
IMagicSkill* MagicSlot::GetEquipped() const
{
	// ‰½‚à‘•”õ‚µ‚Ä‚¢‚È‚©‚Á‚½‚çnullptr‚ğ•Ô‚·
	if (candidates.empty()) return nullptr;

	return candidates[equippedIndex].get();
}

// Œó•â‚Ì”‚ğ•Ô‚·ŠÖ”
int MagicSlot::GetCandidateCount() const
{
	return static_cast<int>(candidates.size());
}

// ‘I‘ğ‚³‚ê‚½‘•”õ‚ğ•Ô‚·ŠÖ”
IMagicSkill* MagicSlot::GetCandidate(int index) const
{
	if (index < 0 || index >= static_cast<int>(candidates.size())) return nullptr;
	return candidates[index].get();
}

// ‘•”õ‚ğØ‚è‘Ö‚¦‚ğ—\–ñ‚·‚éŠÖ”
void MagicSlot::SetEquipped(int index)
{
	// ”ÍˆÍŠO‚Ì”Ô†‚Í–³‹‚·‚é
	if (index < 0 || index >= static_cast<int>(candidates.size())) return;

	pendingIndex = index;
}

// ‘•”õ‚ğØ‚è‘Ö‚¦‚éŠÖ”
void MagicSlot::ApplyPending(const MagicContext& ctx)
{
	if (pendingIndex < 0 || pendingIndex == equippedIndex) { pendingIndex = -1; return; }
	if (IMagicSkill* old = GetEquipped()) old->OnUnequip(ctx);
	equippedIndex = pendingIndex;
	pendingIndex = -1;
}
