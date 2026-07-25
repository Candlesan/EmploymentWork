#pragma once
#define NOMINMAX

#include "GamePlay/Object/Particle/Compute_Particle.h"
#include "GamePlay/Object/Trail/Trail.h"

#include "System/Resource/Model/Model.h"
#include "System/Renderer/ModelRenderer.h"
#include "System/Graphic/RenderContext.h"
#include "System/UI/EffectEditor/EffectConfig.h"
#include <memory>
#include <windows.h>
#include <algorithm>

class EffectEditor
{
public:
	EffectEditor() = default;
	~EffectEditor() = default;

	// 初期化
	void Initialize(ID3D11Device* device);

	// GUI描画
	void DrawGUI();

	// テスト再生
	void PlayTestEffect(const DirectX::XMFLOAT3& spawnPosition);

	// 更新処理
	void Update(ID3D11DeviceContext* dc, float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* modelRenderer);
	void RenderMeshVFX(ModelRenderer* modelRenderer);

private:
	// UIの描画パーツ
	void DrawParticleInspector(ParticleEffectConfig& particleConfig);
	void DrawMeshVfxInspector(MeshVfxConfig& meshVfxConfig);
	void DrawTrailInspector(TrailConfig& trailConfig);
	void DrawFileToolbar();
	void DrawNodeList();


	// ヘルパー関数
	float RandFloat();
	void EmitParticles(float elapsedTime);
private:
	EffectConfig currentConfig; // 現在調整中の設定データ

	// テスト用の描画用のリソース
	std::unique_ptr<Compute_Particle> particleManager;	// パーティクル制御
	std::shared_ptr<Model> vfxMeshModel;				// メッシュVFXモデル
	std::shared_ptr<Trail> trailRenderer;				// トレイル

	// 再生状態
	bool isPlaying = false;
	float currentMeshTime = 0.0f;
	DirectX::XMFLOAT3 testSpawnPos = { 0.0f, 1.0f, 0.0f };

	float effectTimer = 0.0f;	// エフェクト開始からの経過時間

	int selectedNodeIndex = 0; // 選択中のノード
};
