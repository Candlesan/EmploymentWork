#pragma once
#define NOMINMAX

#include "GamePlay/Object/Particle/Compute_Particle.h"
#include "GamePlay/Object/Trail/Trail.h"

#include "System/Resource/Model/Model.h"
#include "System/Renderer/ModelRenderer/ModelRenderer.h"
#include "System/Graphic/RenderContext.h"
#include "System/UI/EffectEditor/EffectConfig.h"
#include <memory>
#include <windows.h>
#include <algorithm>

class CustomEffect;

class EffectEditor
{
public:
	EffectEditor();
	~EffectEditor();

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
	void DrawNodeInspector(NodeConfig& node);
	void DrawFileToolbar();
	void DrawNodeList();

	// ヘルパー関数
	void RebuildPreview(); // 今の設定でプレビュー用のエフェクトを作り直す

private:
	EffectConfig currentConfig; // 現在調整中の設定データ

	// プレビュー再生用のエフェクト（本番と同じ CustomEffect を使う）
	std::unique_ptr<CustomEffect> preview;

	// 再生状態
	DirectX::XMFLOAT3 testSpawnPos = { 0.0f, 1.0f, 0.0f };
	bool isTestMove = false;	// テスト位置を中心に円を描いて動かすか（トレイルの確認用）
	float testMoveTimer = 0.0f;	// 円運動用のタイマー
	int selectedNodeIndex = 0; // 選択中のノード
};