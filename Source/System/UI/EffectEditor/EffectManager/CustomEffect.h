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

		// メッシュ描画用のワールド行列
		DirectX::XMFLOAT4X4 worldTransform = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};

		float spawnTimer = 0.0f;							// 毎フレーム進める発生計算用タイマー
		float currentDelay = 0.0f;          // 今回の再生で使う生成開始時間（Play で決定）
		float currentSpawnInterval = 1.0f;  // 次の発生までの間隔（発生ごとに決定）
		float currentLifeTime = 1.0f;       // メッシュ系の今の周期の寿命（周期ごとに決定）
		DirectX::XMFLOAT4 meshRandomColor = { 1, 1, 1, 1 };	// 色のランダム(計算用)
		DirectX::XMFLOAT4 meshStartColor = { 1, 1, 1, 1 };	// 始点の色(計算用)
		DirectX::XMFLOAT4 meshEndColor = { 1, 1, 1, 1 };	// 終点の色(計算用)
		float meshUniformStartScale = 1.0f;					// 始点の拡大率(計算用)
		float meshUniformEndScale = 1.0f;					// 終点の拡大率(計算用)

		float localTime = 0.0f; // このノードの経過時間
		int parentIndex = -1;   // 親が runTimeNodes の何番目か（-1 = 親なし）
		bool started = false;	// delayTime を過ぎたか
		bool emitting = false;	// パーティクルは発生中か
		bool finished = false;	// このノードはもう終わったか
		bool trailNeedsReset = false;
		std::shared_ptr<Model::Mesh> ringMesh;
	};

public:
	// 初期化
	void Initialize(ID3D11Device* device, const EffectConfig& config);

	// 再生中に設定値だけを差し替える（エディターのプレビュー用）
	// ノード数・描画方法・モデルやテクスチャのパスが変わっていたら false を返す（作り直しが必要）
	bool ApplyConfig(const EffectConfig& newConfig);

	// エフェクト再生
	void Play(
		DirectX::XMFLOAT3 position,
		DirectX::XMFLOAT3 angle,
		DirectX::XMFLOAT3 scale);

	// 常駐モード
	void PlayResident();

	// 指定位置で一回だけパーティクルを発生させる
	void Burst(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& angle, const DirectX::XMFLOAT3& scale);

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

	void CollectLights(std::vector<PointLight>& out) const;

	// ゲッター・セッター
	void SetPosition(const DirectX::XMFLOAT3& pos) { this->position = pos; }
	void SetAngle(const DirectX::XMFLOAT3& rot) { this->angle = rot; }
	void SetScale(const DirectX::XMFLOAT3& scl) { this->scale = scl; }

private:
	void EmitParticles(float elapsedTime);
	void BeginNodeCycle(RuntimeNode& node, bool isFirstCycle);

	DirectX::XMMATRIX ComputeLocalMatrix(const RuntimeNode& node) const;
	DirectX::XMMATRIX ComputeEffectMatrix() const;

	// リングの形をメッシュとして作る
	static std::shared_ptr<Model::Mesh> CreateRingMesh(ID3D11Device* device, const NodeConfig& c);

	// メッシュ1つ分を描く（モデルの各メッシュ・リングで共通）
	void DrawVFXMesh(ID3D11DeviceContext* dc, meshVFX* shader, RenderContext& rc,
		RuntimeNode& node, const Model::Mesh& mesh);

	// 色とフェードをメッシュVFXに掛ける関数
	DirectX::XMFLOAT4 ComputeNodeColor(const RuntimeNode& node) const;

	// ノードのワールド行列を計算
	void UpdateNodeTransforms();

	// 1ノード分のパーティクルを発生させる
	void EmitNode(RuntimeNode& node, float lifeOffset = 0.0f); // Burst からは 0 のまま

private:

	EffectConfig config;
	DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 prevPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 delta = { 0.0f, 0.0f, 0.0f };

	DirectX::XMFLOAT3 scale = { 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 angle = { 0.0f, 0.0f, 0.0f };

	float effectTimer = 0.0f; // 経過時間
	bool isPlaying = false; // エフェクト再生中か
	bool isResident = false; // 常駐モードか
	std::vector<RuntimeNode> runTimeNodes; // 複数ノードに対応した配列
	std::vector<int> updateOrder;
};