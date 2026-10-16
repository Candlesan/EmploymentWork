#include "CustomEffectManager.h"
#include "System/Graphic/Graphics.h"
#include "System/Shader/meshVFX/meshVFX.h"


// 初期化
void CustomEffectManager::Initialize(ID3D11Device* device)
{
	// デバイスを保持する
	this->device = device;

}

// Jsonファイルの読み込み/キャッシュ
void CustomEffectManager::LoadEffectConfig(const std::string& effectName, const std::string& filePath)
{
	// 既に登録されていないかを確認する
	if (effectConfigs.find(effectName) == effectConfigs.end())
	{
		// 登録されていなければ一時的にeffectConfigを作る
		EffectConfig config;
		config.Load(filePath); // Jsonファイルを読み込む

		// 読み込みに成功したら設定を保存する
		effectConfigs[effectName] = config;
	}
}

// エフェクト再生
EffectHandle CustomEffectManager::PlayEffect(const std::string& effectName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale)
{
	auto it = effectConfigs.find(effectName);
	if (it == effectConfigs.end()) return InvalidEffect; // エフェクトが無かったら0を返す

	auto effect = std::make_unique<CustomEffect>();
	effect->Initialize(device, it->second);
	effect->Play(position, angle, scale);

	EffectHandle handle = nextHandle++;
	activeEffects[handle] = std::move(effect);
	return handle;
}

// 更新処理
void CustomEffectManager::Update(float elapsedTime)
{
	// 終わったものをループしながら消す
	for (auto it = activeEffects.begin(); it != activeEffects.end();)
	{
		it->second->Update(elapsedTime);
		
		if (it->second->IsFinished())
		{
			it = activeEffects.erase(it); // 次の要素を消す
		}
		else
		{
			++it;
		}
	}
}

// 描画処理
void CustomEffectManager::Render(RenderContext& rc)
{
	// 全再生中のエフェクトを描画する
	for (auto& [handle, effect] : activeEffects)
	{
		effect->Render(rc);
	}
}

// メッシュVFXの描画
void CustomEffectManager::RenderMeshVFX(ModelRenderer* modelRenderer)
{
	auto* shader = dynamic_cast<meshVFX*>(modelRenderer->GetShader(ShaderId::MeshVFX));

	if (!shader) return;

	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();

	// モデル内の各メッシュを即時描画
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = Graphics::Instance().GetRenderState();
	rc.camera = &CameraManager::Instance().GetMainCamera(); // カメラ参照

	shader->Begin(rc);

	// 全再生中のメッシュVFXを描画する
	for (auto& [handle, effect] : activeEffects)
	{
		effect->RenderMeshVFX(shader, rc);
	}

	shader->End(rc);
}

// 全クリア
void CustomEffectManager::Clear()
{
	activeEffects.clear();
	effectConfigs.clear();
}

std::unique_ptr<CustomEffect> CustomEffectManager::CreateEffect(const std::string& effectName)
{
	// 名前を検索する
	auto it = effectConfigs.find(effectName);
	if (it == effectConfigs.end()) return nullptr;

	auto effect = std::make_unique<CustomEffect>();
	effect->Initialize(device, it->second);

	return effect;
}

bool CustomEffectManager::Exists(EffectHandle handle) const
{
	return activeEffects.find(handle) != activeEffects.end();
}

void CustomEffectManager::Stop(EffectHandle handle)
{
	auto it = activeEffects.find(handle);
	if (it == activeEffects.end()) return; // 既に終わってたら何もしない
	it->second->Stop(); // CustomEffectのStopを呼ぶ
}

void CustomEffectManager::SetPosition(EffectHandle handle, const DirectX::XMFLOAT3& position)
{
	auto it = activeEffects.find(handle);
	if (it != activeEffects.end()) it->second->SetPosition(position);
}

void CustomEffectManager::SetAngle(EffectHandle handle, const DirectX::XMFLOAT3& angle)
{
	auto it = activeEffects.find(handle);
	if (it != activeEffects.end()) it->second->SetAngle(angle);
}

void CustomEffectManager::SetScale(EffectHandle handle, const DirectX::XMFLOAT3& scale)
{
	auto it = activeEffects.find(handle);
	if (it != activeEffects.end()) it->second->SetScale(scale);
}
