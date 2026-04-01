#pragma once
#include "AnimationState.h"
#include "json.hpp"
#include <fstream>
#include <unordered_map>
#include <memory>

template<typename StateEnum>
class AnimationStateManager
{
public:
	static AnimationStateManager& Instance()
	{
		static AnimationStateManager instance;
		return instance;
	}

	const AnimationConfig* GetConfig(StateEnum state) const
	{
		auto it = configs.find(state);
		return (it != configs.end()) ? &it->second : nullptr;
	}

	const std::unordered_map<StateEnum, AnimationConfig>& GetAllConfigs() const
	{
		return configs;
	}

private:
	AnimationStateManager();  // “ÁŽê‰»‚²‚Æ‚É’è‹`
	std::unordered_map<StateEnum, AnimationConfig> configs;
};