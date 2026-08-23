#include "Mathf.h"
#include <stdlib.h>
#include <algorithm>

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

// 0.0 ～ 1.0 に数値を収める
float Mathf::Clamp(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}

// 線形補間
float Mathf::Lerp(float a, float b, float t)
{
	return a + (b - a) * t;
}

// 3次エルミート補間によるイージング
float Mathf::EaseCurve(float t, int startSpeed, int endSpeed)
{
	// はみ出し防止ガード（下限0.0、上限1.0で切り落とす）
	t = Clamp(t);

	// 速度 enum (0:低速3 ～ 3:等速 ～ 6:高速3) を曲線の傾きウェイトに変換
	// 0: 0.0f (完全に寝ている)
	// 3: 1.0f (直線・等速)
	// 6: 3.0f (急激に立ち上がる)
	auto SpeedToWaight = [](int speed)->float
	{
		switch (speed)
		{
		case 0: return 0.0f;
		case 1: return 0.25f;
		case 2: return 0.5f;
		case 3: return 1.0f;
		case 4: return 1.5f;
		case 5: return 2.0f;
		case 6: return 3.0f;
		default: return 1.0f;
		}
	};

	float v0 = SpeedToWaight(startSpeed);
	float v1 = SpeedToWaight(endSpeed);

	// どちらも等速ならそのまま直線
	if (startSpeed == 3 && endSpeed == 3)
	{
		return t;
	}

	// 3次エルミート基本関数によるブレンド
	// P(t) = (2t^3 - 3t^2 + 1)*P0 + (t^3 - 2t^2 + t)*V0 + (-2t^3 + 3t^2)*P1 + (t^3 - t^2)*V1
	// (P0 = 0.0, P1 = 1,0 と置いた簡易式)

	float t2 = t * t;
	float t3 = t2 * t;

	//(v1 - 1.0f)の1.0はベースを直線tにして傾きの変化分だけ足す形にするため
	//- 1.0f は「傾きの基準値（等速 = 1.0）との差」を表す変形の名残
	//v1(t^3 - t^2) = (v_1 - 1.0)(t^3 - t^2) + 1.0(t^3 - t^2)
	float result = (3.0f * t2 - 2.0f * t3) + v0 * (t3 - 2.0f * t2 + t) + (v1 - 1.0f) * (t3 - t2);

	return Clamp(result);


}

