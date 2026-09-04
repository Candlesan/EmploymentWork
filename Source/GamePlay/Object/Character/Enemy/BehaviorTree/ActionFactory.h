#pragma once
#include <string>

// ‘O•ûéŒ¾
class ActionBase;
class Enemy;

enum class BTActionType;

class ActionFactory
{
public:
	static ActionBase* Creat(
		BTActionType type,
		Enemy* enemy,
		const std::string& stateName);
};