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
