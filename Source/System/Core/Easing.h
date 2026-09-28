#pragma once
#include <DirectXMath.h>


enum EasingType
{
	Linear = 0,
	EaseInCubic,
	EaseOutCubic, // スタートがより早く、直前にブレーキがかかるようなメリハリがある
	EaseInOutCubic,
	EaseOutBack, // 跳ね返り
	EaseOutBounce, // バウンド用
};

class Easing
{
public:
	static float Ease(EasingType type, float t);

	static float Linear(float t);
	static float EaseInCubic(float t);
	static float EaseOutCubic(float t);
	static float EaseInOutCubic(float t);
	static float EaseOutBack(float t);
	static float EaseOutBounce(float t);
};
