#pragma once
#include <DirectXMath.h>
#include <cstdlib>

// 浮動小数算術
class Mathf
{
public:
	// 指定範囲のランダム値を計算する
	static float RandomRange(float min, float max);
	static int RandomRange(int min, int max);
	static float RandFloat();

	// 0.0 ～ 1.0 に数値を収める
	static float Clamp(float value);

	// 線形補間
	static float Lerp(float a, float b, float t);

	static DirectX::XMFLOAT2 Lerp(const DirectX::XMFLOAT2& a, const DirectX::XMFLOAT2& b, float t);

	// 長さを返す関数
	static float Length(const DirectX::XMFLOAT3& vel);

	// 自作エフェクト用イージング関数 (t: 0.0~1.0, startSpeed: 0~6, endSpeed: 0~6)
	static float EaseCurve(float t, int startSpeed, int endSpeed);

	// 範囲：-1～1
	static float GetRandRange() { return static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f; }
};