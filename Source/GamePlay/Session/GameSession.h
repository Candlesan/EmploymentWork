#pragma once
#include "GamePlay/Object/Magic/Loadout/MagicInputController.h"


// ゲーム進行の情報
class GameSession
{
public:
	static GameSession& Instance()
	{
		static GameSession instance;
		return instance;
	}

	// ビルド
	int equippedIndex[MAGIC_SLOT_COUNT] = {}; // スロットごとの装備番号

	// 今回のプレイの結果
	float clearTime = 0.0f;
	bool isCleared = false;

	// スタート画面に戻った時
	void ResetAll()
	{
		for (auto& i : equippedIndex) i = 0;
		ResetRun();
	}

	// ゲーム開始時
	void ResetRun()
	{
		clearTime = 0.0f;
		isCleared = false;
	}
};