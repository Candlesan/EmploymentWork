#pragma once
#include "System/Core/Easing.h"
#include "System/Core/Mathf.h"

// ループの種類
enum class LoopType
{
	Once, // 一回だけ
	Loop, // 終わったら最初から
	PingPong, // 終わったら逆向きに
};

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

	LoopType loop = LoopType::Once;
	int loopCount = -1;     // ループ回数
	bool reverse = false;   // 逆再生
	int remaining = 0;      // 残り回数

	bool enabled = false; // このアニメーションを使用するか

	// 再生開始
	// 読むだけで書き換えないって意味にするためにconstを付ける
	void Start(const T& from, const T& to, float duration,
		EasingType ease = EasingType::Linear,
		LoopType loop = LoopType::Once,
		int loopCount = -1)
	{
		this->from = from;
		this->to = to;
		this->duration = duration;
		this->ease = ease;
		this->loop = loop;
		this->loopCount = loopCount;
		remaining = this->loopCount;
		timer = 0.0f;
		reverse = false;
		playing = true;
	}

	// 更新
	void Update(float elapsedTime)
	{
		// 再生中だけ時間を進める
		if (!playing) return;

		timer += elapsedTime;
		if (timer < duration) return; // まだ途中

		// 1週が終わった
		if (loop == LoopType::Once || duration == 0.0f)
		{
			timer = duration;
			playing = false;
			return;
		}

		// 回数を減らす
		if (remaining > 0 && --remaining == 0)
		{
			timer = duration;
			playing = false;
			return;
		}

		// はみ出た時間次の週に持ち越す
		timer -= duration;

		// 往復なら逆向きにする
		if (loop == LoopType::PingPong) reverse = !reverse;
	}

	// 現在の値を返す
	T GetValue() const
	{
		// 時間の進み具合を計算する
		float t = (duration > 0.0f) ? Mathf::Clamp(timer / duration) : 1.0f;

		if (reverse) t = 1.0f - t; // 逆向きなら進み具合をひっくり返す

		// イージングを掛ける
		t = Easing::Ease(ease, t);

		// 型ごとの補間を呼ぶ
		return Mathf::Lerp(from, to, t);
	}

	void Play()
	{
		if (!enabled) return;
		Start(from, to, duration, ease, loop, loopCount);
	}

	void Stop()
	{
		playing = false;
	}
};
