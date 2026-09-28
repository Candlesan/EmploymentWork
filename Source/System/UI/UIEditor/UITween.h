#pragma once
#include "System/Core/Easing.h"

// 1つの値を時間をかけて動かす
template<typename T>
struct UITween
{
	T from{};				// 開始の値
	T to{};					// 終了の値
	float duration = 0.0f;	// かける時間(秒)
	float timer = 0.0f;		// 経過時間
	bool playing = false;	// 再生中か

	EasingType ease = EasingType::Linear; // イージング

	// 再生開始
	// 読むだけで書き換えないって意味にするためにconstを付ける
	void Start(const T& from, const T& to, float duration)
	{
		this->from = from;
		this->to = to;
		this->duration = duration;
		timer = 0.0f;
		playing = true;
	}

	// 更新
	void Update(float elapsedTime)
	{
		// 再生中だけ時間を進める
		if (!playing) return;

		timer += elapsedTime;

		// 終わったら止める
		if (timer >= duration)
		{
			timer = duration;
			playing = false;
		}
	}

	// 現在の値を返す
	T GetValue() const
	{
		// 時間の進み具合を計算する
		float t = (duration > 0.0f) ? timer / duration : 1.0f;

		// イージングを掛ける
		t = Easing::Ease(ease, t);

		// 型ごとの補間を呼ぶ
		return Mathf::Lerp(from, to, t);
	}
};
