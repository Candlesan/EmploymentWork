#pragma once
#include "System/Graphic/RenderContext.h"
#include "System/UI/EffectEditor/EffectConfig.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffect.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <DirectXMath.h>

class CustomEffectManager
{
public:
	// シングルトン
	static CustomEffectManager& Instance()
	{
		static CustomEffectManager instance;
		return instance;
	}

	// 初期化
	void Initialize(ID3D11Device* device);

	// Jsonファイルの読み込み/キャッシュ
	void LoadEffectConfig(const std::string& effectName, const std::string& filePath);

	// エフェクト再生
	void PlayEffect(
		const std::string& effectName,
		DirectX::XMFLOAT3 position,
		DirectX::XMFLOAT3 angle = {0.0f, 0.0f, 0.0f},
		DirectX::XMFLOAT3 scale = {1.0f, 1.0f, 1.0f});

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(RenderContext& rc);
	void RenderMeshVFX(ModelRenderer* modelRenderer);

	// 全部クリア
	void Clear();
private:
	CustomEffectManager() = default;
	~CustomEffectManager() { Clear(); }

private:
	ID3D11Device* device;
	std::unordered_map<std::string, EffectConfig> effectConfigs; // キャッシュ用の箱
	std::vector<CustomEffect*> activeEffects; // 現在再生中のエフェクト一覧
};
