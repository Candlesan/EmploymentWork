#pragma once
#include "System/Graphic/RenderContext.h"
#include "System/UI/EffectEditor/EffectConfig.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffect.h"

#include <string>
#include <vector>
#include <map>
#include <DirectXMath.h>

using EffectHandle = int; // 0は無効になる
constexpr EffectHandle InvalidEffect = 0;

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

	// エフェクト再生関数
	EffectHandle PlayEffect(const std::string& effectName,
		DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 angle, DirectX::XMFLOAT3 scale);

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(RenderContext& rc);
	void RenderMeshVFX(ModelRenderer* modelRenderer);

	// 全部クリア
	void Clear();

	// 設定済みにのエフェクトをインスタンス化してポインタを返す関数
	std::unique_ptr<CustomEffect> CreateEffect(const std::string& effectName);

	bool Exists(EffectHandle handle) const; // まだ生きているか
	void Stop(EffectHandle handle);	// 停止関数
	void SetPosition(EffectHandle handle, const DirectX::XMFLOAT3& position);
	void SetAngle(EffectHandle handle, const DirectX::XMFLOAT3& angle);
	void SetScale(EffectHandle handle, const DirectX::XMFLOAT3& scale);

private:
	CustomEffectManager() = default;
	~CustomEffectManager() { Clear(); }

private:
	ID3D11Device* device;

	std::unordered_map<std::string, EffectConfig> effectConfigs; // キャッシュ用の箱

	// Effekseerみたいにハンドルに変更
	std::map<EffectHandle, std::unique_ptr<CustomEffect>> activeEffects;
	EffectHandle nextHandle = 1;
};
