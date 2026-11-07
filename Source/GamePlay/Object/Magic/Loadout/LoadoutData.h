#pragma once
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"

class LoadoutData
{
public:
	static LoadoutData& Instance()
	{
		static LoadoutData instance;
		return instance;
	}

	int equippedIndex[MAGIC_SLOT_COUNT] = {}; // ƒXƒƒbƒg‚²‚Æ‚Ì‘•”õ”Ô†
};