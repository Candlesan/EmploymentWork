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
	void DrawStateRangeTimeline();
	void DrawNodeInspector(NodeConfig& node);
	void DrawFileToolbar();
	void DrawNodeList();

	// ヘルパー関数
	float RandFloat();
	void EmitParticles(float elapsedTime);
	void SyncRuntimeNodes();
	bool HasRange(std::string name);
private:
	// エディター再生・プレビュー用の1ノード分リソース
	struct RuntimeNode
	{
		std::shared_ptr<Model> meshModel = nullptr;  // そのノード専用の3Dモデル
		std::shared_ptr<Trail> trail = nullptr;      // そのノード専用のトレイル
		DirectX::XMFLOAT2 currentUvOffset = { 0.0f, 0.0f }; // UV加算用
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView; // テクスチャ用のシェーダーリソースビュー
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noiseShaderResourceView; // ノイズテクスチャ用シェーダーリソースビュー
	};

	EffectConfig currentConfig; // 現在調整中の設定データ

	// テスト用の描画用のリソース
	std::unique_ptr<Compute_Particle> particleManager;	// パーティクル制御
	std::vector<RuntimeNode> runtimeNodes;        // 各ノード専用のリソース配列

	// 再生状態
	DirectX::XMFLOAT3 testSpawnPos = { 0.0f, 1.0f, 0.0f };
	char rangeNameBuf[64]{}; // 編集用バッファ
	int selectedNodeIndex = 0; // 選択中のノード
	int selectedRangeIndex = -1; // 選択中のStateRangeインデックス
	float currentMeshTime = 0.0f;
	float effectTimer = 0.0f;	// エフェクト開始からの経過時間
	float globalAlpha = 1.0f;
	bool isPlaying = false;
	bool isStop = false;
};
