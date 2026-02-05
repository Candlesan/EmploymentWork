#include<stdlib.h>
#include"Mathf.h"

float Mathf::RandomRange(float min, float max)
{
	//0.0～1.0の間までのランダム値
	float value = static_cast<float>(rand()) / RAND_MAX;

	//min～maxまでのランダム値に変換
	return min + (max - min) * value;
}