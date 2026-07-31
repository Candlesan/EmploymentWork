#include <stdlib.h>
#include "Mathf.h"

float Mathf::RandomRange(float min, float max)
{
	// 0.0～1.0の間までのランダム値
	float value = static_cast<float>(rand()) / RAND_MAX;

	// min～maxまでのランダム値に変換
	return min + (max - min) * value;
}

int Mathf::RandomRange(int min, int max)
{
	// 0.0～1.0の間までのランダム値
	int value = static_cast<int>(rand()) / RAND_MAX;

	// min～maxまでのランダム値に変換
	return min + (max - min) * value;
}

// -1.0f～1.0fの間をランダムで返す関数
float Mathf::RandFloat()
{
	return (static_cast<float>(rand()) / (static_cast<float>(RAND_MAX))) * 2.0f - 1.0f;
}

