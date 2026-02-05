#pragma once

#include"ModelRenderer.h"
#include<memory>

#include"Character.h"

class BehaviorTree;
class BehaviorData;
class NodeBase;

//エネミー
class Enemy : public Character
{
public:
	Enemy();
	~Enemy();

	//更新
	void Update(float elapsedTime);
	//描画
	void Render(const RenderContext& rc, ModelRenderer* renderer);
	//GUI描画
	void DrawGui();

	//移動ベクトル取得
	DirectX::XMFLOAT3 GetMoveVec();

	// アニメーション関連 //
	//アニメーションステート
	enum class AnimationState
	{
		Idle,
		Walk,
	};
	//アニメーション更新処理
	void UpdateAnimations(float elapsedTime);
	//アニメーションが終了したか
	bool FinshedAnimation();
	//アニメーションステートセッター
	void SetAnimationState(AnimationState SetState);

	// AI関連 //
	//実行タイマー取得
	const float GetRunTimer() { return runTimer; }
	//実行タイマー設定
	void SetRunTimer(float timer) { runTimer = timer; }
	//目的地ランダム設定(一旦)
	void SetRandomTargetPosition();
	//目的地取得
	DirectX::XMFLOAT3 GetTargetPosition() { return targetPosition; }
	//目的地へ移動(一旦)
	void MoveToTarget(float elapsedTime, float speedRate);
private:
	std::shared_ptr<Model>enemy;

	//ツリー関係
	BehaviorTree* aiTree = nullptr;
	BehaviorData* behaviorData = nullptr;
	NodeBase* activeNode = nullptr;

	//実行タイマー
	float runTimer;
	//目的地
	DirectX::XMFLOAT3 targetPosition;
	//目的範囲
	float targetRange = 10.0f;
	//移動速度
	float moveSpeed = 3.0f;
	//回転速度
	float turnSpeed = DirectX::XMConvertToRadians(360);

	// アニメーション関係 //
	AnimationState animationState = AnimationState::Idle;
	//現在の姿勢
	std::vector<Model::NodePose>nodePoses;
	//前フレームの姿勢
	std::vector<Model::NodePose>oldNodePoses;
	//現在のアニメーション
	float animationIndex = -1;
	//現在の再生時間
	float animationSeconds = 0;
	//前フレームの再生時間
	float oldAnimationSeconds = 0;
	//アニメーションの補完するか
	bool isBlending = true;
	//補完する時間
	float animationBlendSeconds = 0.2f;
	//補完する長さ
	float animationBlendSecondsLength = 0.2f;
	//ループするか
	bool animationLoop = false;
	//ルートモーションをするか
	bool useRootMotion = false;
	//ルートモーションExをするか
	bool useRootMotionEx = false;
	//Y軸を無視するか
	bool bakeTranslationY = false;

};
