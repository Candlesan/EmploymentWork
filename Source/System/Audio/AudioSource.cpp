#include "System/Core/Misc.h"
#include "AudioSource.h"

// コンストラクタ
AudioSource::AudioSource(IXAudio2* xaudio, std::shared_ptr<AudioResource>& resource)
	: resource(resource)
{
	HRESULT hr;

	// ソースボイスを生成
	hr = xaudio->CreateSourceVoice(&sourceVoice, &resource->GetWaveFormat());
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
}

// デストラクタ
AudioSource::~AudioSource()
{
}

// 終了化
void AudioSource::Finalize()
{
	if (sourceVoice) {
		sourceVoice->Stop(0);
		sourceVoice->DestroyVoice();
		sourceVoice = nullptr;
	}
}

// 再生
void AudioSource::Play(bool loop)
{
	Stop();

	// ソースボイスにデータを送信
	XAUDIO2_BUFFER buffer = { 0 };
	buffer.AudioBytes = resource->GetAudioBytes();
	buffer.pAudioData = resource->GetAudioData();
	buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	
	sourceVoice->SubmitSourceBuffer(&buffer);

	HRESULT hr = sourceVoice->Start();
	_ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
	sourceVoice->SetVolume(1.0f);
}

// 停止
void AudioSource::Stop()
{
	sourceVoice->FlushSourceBuffers();
	sourceVoice->Stop();
}

// 一時停止
void AudioSource::Pause()
{
	if (sourceVoice)
	{
		// 再生を停止（バッファは保持）
		sourceVoice->Stop(0);
	}
}

// 再開
void AudioSource::Resume()
{
	if (sourceVoice)
	{
		// 停止状態から再開
		sourceVoice->Start(0);
	}
}

bool AudioSource::IsPlaying() const
{
	if (!sourceVoice) return false;

	XAUDIO2_VOICE_STATE state;
	sourceVoice->GetState(&state);

	// キューに入っているバッファがあれば再生中
	return state.BuffersQueued > 0;
}

bool AudioSource::IsFinished() const
{
	// 再生中でなければ終了している
	return !IsPlaying();
}

// 音量設定
void AudioSource::SetVolume(float volume)
{
	sourceVoice->SetVolume(volume);
}