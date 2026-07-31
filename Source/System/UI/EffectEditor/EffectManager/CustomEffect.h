#pragma once
#define NOMINMAX
#include "System/UI/EffectEditor/EffectConfig.h"
#include "System/UI/EffectEditor/EffectEditor.h"

#include <memory>
#include <algorithm>

class CustomEffect
{
public:
	// 初期化
	void Initialize(ID3D11Device* device, const EffectConfig& config);

	// エフェクト再生
	void Play(
		DirectX::XMFLOAT3 position,
		DirectX::XMFLOAT3 angle,
		DirectX::XMFLOAT3 scale);

	// エフェクト更新
	void Update(float elapsedTime);

	// エフェクト描画
	void Render(RenderContext& rc);
	void RenderMeshVFX(ModelRenderer* modelRenderer);


	// エフェクト終了関数
	bool IsFinished();

	// 設置位置調整用にGUI
	void DrawGUI();

private:
	void EmitParticles(float elapsedTime);

private:
	EffectConfig config;
	DirectX::XMFLOAT3 position, scale, angle = { 0.0f, 0.0f, 0.0f };
	float effectTimer = 0.0f; // 経過時間
	bool isPlaying = false; // エフェクト再生中か
	std::unique_ptr<Compute_Particle> particleManager; // パーティクル用
	std::shared_ptr<Model> vfxMeshModel; // メッシュVFX用
	std::shared_ptr<Trail> trailRenderer; // トレイル用
};

