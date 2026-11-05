#pragma once
#include "GamePlay/Object/Magic/IMagicSkill.h"

#include <vector>
#include <memory>

class MagicSlot
{
public:
	// Œó•â‚ğ‘«‚·ŠÖ”
	void AddCandidate(std::unique_ptr<IMagicSkill> skill);

	// ‘•”õ’†‚Ì‹Z‚ğ•Ô‚·ŠÖ”
	IMagicSkill* GetEquipped() const;

	// ‘•”õ‚ğØ‚è‘Ö‚¦‚éŠÖ”
	void SetEquipped(int index);

private:
	std::vector<std::unique_ptr<IMagicSkill>> candidates; // Œó•â‚Ì‹ZƒŠƒXƒg
	int equippedIndex = 0; // ¡‚Ç‚ê‚ğ‘•”õ‚µ‚Ä‚¢‚é‚©
};