#pragma once
#include <string>

// アニメーションの設定
struct AnimationConfig
{
	std::string animationName = "";	// アニメーション名
	bool loop = false;				// アニメーションをループするか
	bool useRootMotion = false;		// ルートモーションするか
	bool useRootMotionEx = false;	// 腰骨に対応したルートモーション
	float blendTime = 0.2f;			// 補間時間
};
