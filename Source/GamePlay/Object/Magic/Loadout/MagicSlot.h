#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"

#include <vector>
#include <memory>

class MagicSlot
{
public:
	// Œó•â‚ğ‘«‚·ŠÖ”
	void AddCandidate(std::unique_ptr<IMagicSkill> skill);

	// Œó•â‚Ì”‚ğ•Ô‚·ŠÖ”
	int GetCandidateCount() const;

	// ‘I‘ğ‚³‚ê‚½‘•”õ‚ğ•Ô‚·ŠÖ”
	IMagicSkill* GetCandidate(int index) const;

	// ‘•”õ’†‚Ì‹Z‚ğ•Ô‚·ŠÖ”
	IMagicSkill* GetEquipped() const;

	// ¡‘•”õ‚µ‚Ä‚¢‚é”Ô†‚ğ•Ô‚·ŠÖ”
	int GetEquippedIndex() const { return equippedIndex; }

	// ‘•”õ‚ğØ‚è‘Ö‚¦‚é‚ğ—\–ñ‚·‚éŠÖ”
	void SetEquipped(int index);

	//@‘•”õ‚ğØ‚è‘Ö‚¦‚éŠÖ”
	void ApplyPending(const MagicContext& ctx);

	void Clear()
	{
		candidates.clear();
		equippedIndex = 0;
		pendingIndex = -1;
	}

private:
	std::vector<std::unique_ptr<IMagicSkill>> candidates; // Œó•â‚Ì‹ZƒŠƒXƒg
	int equippedIndex = 0; // ¡‚Ç‚ê‚ğ‘•”õ‚µ‚Ä‚¢‚é‚©
	int pendingIndex = -1;
};