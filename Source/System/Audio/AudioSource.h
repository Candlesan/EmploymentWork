#pragma once

#include <memory>
#include <xaudio2.h>
#include "System/Audio/AudioResource.h"

// オーディオソース
class AudioSource
{
public:
	AudioSource(IXAudio2* xaudio, std::shared_ptr<AudioResource>& resource);
	~AudioSource();

	void Finalize();

	// 再生
	void Play(bool loop);

	// 停止
	void Stop();

	// 音量設定
	void SetVolume(float volume);

	// 一時停止
	void Pause();

	// 再開
	void Resume();

	// 再生中かどうかをチェック
	bool IsPlaying() const;

	// 再生が完了したかをチェック（ループしていない音声用）
	bool IsFinished() const;

private:
	IXAudio2SourceVoice*			sourceVoice = nullptr;
	std::shared_ptr<AudioResource>	resource;
};
