#include "CustomEffectManager.h"

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
void CustomEffectManager::PlayEffect(const std::string& effectName, DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale)
{
	// エフェクト名で検索する
	auto it = effectConfigs.find(effectName);

	// 見つからなかったら処理をしない
	if (it == effectConfigs.end()) return;

	// 新しくエフェクトを作る
	auto effect = new CustomEffect();

	// 読み込んでおいた設定とデバイスを渡して初期化する
	effect->Initialize(device, it->second);

	// エフェクトを再生する
	effect->Play(position, angle, scale);

	// 再生中のエフェクトをリストに追加する
	activeEffects.push_back(effect);
}

// 更新処理
void CustomEffectManager::Update(float elapsedTime)
{
	// 破棄するエフェクトを一時的にためるリスト
	std::vector<CustomEffect*> removes;

	// 全エフェクトを更新する
	for (auto& effect : activeEffects)
	{
		effect->Update(elapsedTime);

		// 終了しているエフェクトが無いか確認する
		if (effect->IsFinished())
		{
			removes.push_back(effect); // 破棄リストに追加
		}
	}

	// 更新が終わった後に削除処理をする
	for (auto& effect : removes)
	{
		auto it = std::find(activeEffects.begin(), activeEffects.end(), effect);
		if (it != activeEffects.end())
		{
			activeEffects.erase(it); // リストから除外
		}

		delete effect; // エフェクトを解放
	}
}

// 描画処理
void CustomEffectManager::Render(RenderContext& rc)
{
	// 全再生中のエフェクトを描画する
	for (auto effect : activeEffects)
	{
		effect->Render(rc);
	}
}

// メッシュVFXの描画
void CustomEffectManager::RenderMeshVFX(ModelRenderer* modelRenderer)
{
	// 全再生中のメッシュVFXを描画する
	for (auto effect : activeEffects)
	{
		effect->RenderMeshVFX(modelRenderer);
	}
}

// 全クリア
void CustomEffectManager::Clear()
{
	for (auto effects : activeEffects)
	{
		delete effects;
	}
	activeEffects.clear();
	effectConfigs.clear();
}