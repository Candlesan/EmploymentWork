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

// 常駐エフェクトを取得する(無ければつくる)
CustomEffect* CustomEffectManager::GetOrCreateResident(const std::string& effectName)
{
	// こののエフェクトがあるか探す
	auto it = residentEffects.find(effectName);
	if (it != residentEffects.end()) return it->second.get(); // 既にあったら見つけたのを返す

	// エフェクトを生成する
	auto effect = CreateEffect(effectName);
	if (!effect) return nullptr;

	effect->PlayResident();
	CustomEffect* ptr = effect.get();
	residentEffects[effectName] = std::move(effect); // 所有権を渡してそれを返す

	return ptr;
}

// 常駐エフェクトを使って指定位置に1回だけ発生させる
void CustomEffectManager::PlayBurst(const std::string& effectName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale)
{
	CustomEffect* effect = GetOrCreateResident(effectName);
	if (effect) effect->Burst(position, angle, scale);
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

	// 常駐エフェクトの更新
	for (auto& [name, effect] : residentEffects)
	{
		effect->Update(elapsedTime);
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

	for (auto& [name, effect] : residentEffects)
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

	for (auto& [name, effect] : residentEffects)
	{
		effect->RenderMeshVFX(shader, rc);
	}

	shader->End(rc);
}

// 全クリア
// 再生中のエフェクトだけ全部消す（読み込んだ設定は残す）
void CustomEffectManager::StopAll()
{
	activeEffects.clear();
}

void CustomEffectManager::Clear()
{
	activeEffects.clear();
	effectConfigs.clear();
	residentEffects.clear();
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

void CustomEffectManager::CollectLights(LightManager& lightManager) const
{
	std::vector<PointLight> lights;
	for (const auto& [handle, effect] : activeEffects)   effect->CollectLights(lights);
	for (const auto& [name, effect] : residentEffects) effect->CollectLights(lights);

	for (const PointLight& l : lights) lightManager.AddFramePointLight(l);
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
