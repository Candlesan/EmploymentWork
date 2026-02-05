#pragma once
#include "ModelRenderer.h"
#include "Character.h"
#include <memory>

// プレイヤー
class Player : public Character
{
public:
	Player();
	~Player() override {};

	// 更新処理
	void Update(float elapsedTime);

	// 描画処理
	void Render(const RenderContext& rc, ModelRenderer* renderer);

	// GUI描画
	void DrawGui();

	// 移動入力処理
	void InputMove(float elapsedTime);

	//スティック入力またはマウスの角度から8方向のいずれかを判断して攻撃
	void Attack();

private:
	// スティック入力値から移動ベクトルを取得
	DirectX::XMFLOAT3 GetMoveVec();

	// アニメーション更新処理
	void UpdateAnimations(float elapsedTime);

	// アニメーションが終了したか
	bool FinshedAnimation();

private:
	std::shared_ptr<Model> player; // モデル

	float moveSpeed = 5.0f; // 移動速度
	float turnSpeed = DirectX::XMConvertToDegrees(720); // 回転速度

	// アニメーション関係
	enum class State
	{
		Idle = 0,
		Walk,
		Run,
	};

	State state = State::Idle;
	DirectX::XMFLOAT4X4					worldTransform = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	std::vector<Model::NodePose> nodePoses;		// 現在の姿勢
	std::vector<Model::NodePose> oldNodePoses;	// 前フレームの姿勢
	int animationIndex = -1;					// 現在のアニメーション
	float animationSeconds = 0;					// 現在の再生時間
	float oldAnimationSeconds = 0;				// 前フレームの再生時間
	float animationBlendSeconds = 0.2f;			// 補間する時間
	float animationBlendSecondsLength = 0.2f;	// 補間する長さ
	bool isBlending = true;						// アニメーションの補間をするか
	bool animationLoop = false;					// ループするか
	bool useRootMotion = false;					// ルートモーションをするか
	bool useRootMotionEx = false;				// ルートモーションExをするか
	bool bakeTranslationY = false;				// Y軸を無視するか
	std::string debug_attack_log = "No Input";  // 攻撃方向の計算結果をGUIに表示するための文字列を保持
};
