#pragma once
#define NOMINMAX

#include "GamePlay/Object/Particle/Compute_Particle.h"

#include "System/UI/EffectEditor/EffectConfig.h"
#include "System/UI/EffectEditor/EffectEditor.h"

#include <memory>
#include <algorithm>

class meshVFX;

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
	void RenderMeshVFX(meshVFX* shader, RenderContext& rc);


	// エフェクト終了関数
	bool IsFinished();

	// 停止
	void Stop();

	// 設置位置調整用にGUI
	void DrawGUI();

	// ゲッター・セッター
	void SetPosition(const DirectX::XMFLOAT3& pos) { this->position = pos; }
	void SetAngle(const DirectX::XMFLOAT3& rot) { this->angle = rot; }
	void SetScale(const DirectX::XMFLOAT3& scl) { this->scale = scl; }


private:
	void EmitParticles(float elapsedTime);

private:
	// 実行時（再生中）の1ノード分のインスタンス
	struct RuntimeNode
	{
		NodeConfig config; // 設定データ
		std::shared_ptr<Model> meshModel = nullptr;
		std::shared_ptr<Trail> trail = nullptr;
		std::unique_ptr<Compute_Particle> particleManager;	// パーティクル制御
		DirectX::XMFLOAT2 currentUvOffset = { 0.0f, 0.0f };	// 毎フレーム加算していくUV位置
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView; // テクスチャ用のシェーダーリソースビュー
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> noiseShaderResourceView; // ノイズテクスチャ用シェーダーリソースビュー
	};


	EffectConfig config;
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 angle = { 0.0f, 0.0f, 0.0f };

	float effectTimer = 0.0f; // 経過時間
	bool isPlaying = false; // エフェクト再生中か
	std::vector<RuntimeNode> runTimeNodes; // 複数ノードに対応した配列！
};

