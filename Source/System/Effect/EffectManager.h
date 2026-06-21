#pragma once

#include <Effekseer.h>
#include <EffekseerRendererDX11.h>
#include <DirectXMath.h>
#include <string>
#include <unordered_map>
#include <memory>

// 前方宣言
class Effect;

//エフェクトマネージャー
class EffectManager
{
private:
	EffectManager();
	~EffectManager();

public:
	//唯一のインスタンス取得
	static EffectManager& Instance();

	//初期化
	void Initialize();

	//終了化
	void Finalize();

	//更新処理
	void Update(float elapsedTime);

	//描画処理
	void Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection);

	//Effeckseerマネージャーの取得
	Effekseer::ManagerRef GetEffekseerManager() { return effekseerManager; }

	// エフェクトを取得無ければロードする
	Effect* GetOrLoadEffect(const std::string& fileName);

private:
	Effekseer::ManagerRef effekseerManager;
	EffekseerRenderer::RendererRef effekseerRenderer;

	// 読み込み済みのエフェクトを保存しておくやつ
	std::unordered_map<std::string, std::unique_ptr<Effect>> effectCache;
};
