#include "System/Graphic/Graphics.h"
#include "System/Effect/Effect.h"
#include "System/Effect/EffectManager.h"

//コンストラクタ
Effect::Effect(const char* filename)
{
    OutputDebugStringA("Effectコンストラクタ開始\n");

    std::lock_guard<std::mutex> lock(Graphics::Instance().GetMutex());
    OutputDebugStringA("ロック取得\n");

    char16_t utf16Filename[256];
    Effekseer::ConvertUtf8ToUtf16(utf16Filename, 256, filename);
    OutputDebugStringA("UTF16変換完了\n");

    Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();
    OutputDebugStringA("マネージャー取得完了\n");

    effekseerEffect = Effekseer::Effect::Create(effekseerManager, (EFK_CHAR*)utf16Filename);
    OutputDebugStringA("Effect::Create完了\n");
}

//再生
Effekseer::Handle Effect::Play(const DirectX::XMFLOAT3& position, float scale)
{
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

	Effekseer::Handle handle = effekseerManager->Play(effekseerEffect, position.x,
		position.y, position.z);
	effekseerManager->SetScale(handle, scale, scale, scale);
	return handle;
}

//停止
void Effect::Stop(Effekseer::Handle handle)
{
	Effekseer::ManagerRef effekseerManager = EffectManager::Instance().GetEffekseerManager();

	effekseerManager->StopEffect(handle);
}

//座標設定
void Effect::SetPosition(Effekseer::Handle handle, const DirectX::XMFLOAT3& position)
{
	Effekseer::ManagerRef effkeseerManager = EffectManager::Instance().GetEffekseerManager();

	effkeseerManager->SetLocation(handle, position.x, position.y, position.z);
}

//スケール設定
void Effect::SetScale(Effekseer::Handle handle, const DirectX::XMFLOAT3& scale)
{
	Effekseer::ManagerRef effkeseerManager = EffectManager::Instance().GetEffekseerManager();

	effkeseerManager->SetScale(handle, scale.x, scale.y, scale.z);
}
