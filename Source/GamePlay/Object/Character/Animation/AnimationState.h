#pragma once
#include <string>

// アニメーションの設定
struct AnimationConfig
{
	// JSONのキーと合わせるための名前（ステート名）
	std::string stateName;
	// モデル内の実際のアニメーション名
	std::string animationName;
	bool loop;
	bool useRootMotion;
	bool useRootMotionEx;
	float blendTime;

	AnimationConfig()
		:stateName(""), animationName(""), loop(false),
		useRootMotion(false), useRootMotionEx(false), blendTime(0.2f) {}

	AnimationConfig(const std::string& name, bool isLoop = false, bool rootMotion = false, bool rootMotionEx = false, float blend = 0.2f)
		: animationName(name), loop(isLoop), useRootMotion(rootMotion), useRootMotionEx(rootMotionEx), blendTime(blend) {
	}
};