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

// ‘•”õ‚ğØ‚è‘Ö‚¦‚éŠÖ”
void MagicSlot::SetEquipped(int index)
{
	// ”ÍˆÍŠO‚Ì”Ô†‚Í–³‹‚·‚é
	if (index < 0 || index >= static_cast<int>(candidates.size())) return;

	equippedIndex = index;
}