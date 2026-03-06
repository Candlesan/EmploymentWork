#pragma once
#include "GamePlay/Object/Character/Character.h"
#include "AnimationState.h"
#include <vector>
#include <memory>

// アニメーション機能を持つキャラクター基底クラス
template<typename StateEnum>
class AnimationCharacter : public Character
{
public:
	AnimationCharacter() = default;
	virtual ~AnimationCharacter() = default;

	// ステート変更
	void ChangeAnimationState(StateEnum newState, bool ignoreOverlay = false);

	// 上半身のアニメーション
	void StartOverlayAnimation(StateEnum newState);

	// アニメーション更新
	void UpdateAnimation(float elapsedTime);

	// アニメーション終了判定
	bool IsAnimationFinished() const;

	// 再生時間で判定する関数（特定の範囲内なら遷移OK）
	bool IsAnimationInTimeRange(float startSeconds, float endSeconds) const;

	// 再生時間で判定する関数（特定の範囲外なら遷移OK）
	bool IsAnimationOutTimeRange(float startSeconds, float endSeconds) const;

	// 再生時間で判定する関数（特定の場所以降なら遷移OK）
	bool IsAnimationOutTimeRange(float StartTransition) const;

	// だんだんアニメーション遅くする関数
	void AnimationLerp(float StartSlow, float endSlow, float SlowSpeed);

	// このノードを起点に子ノードが上半身の子かを判断する関数
	bool IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName);

	// 現在のステート取得
	StateEnum GetCurrentState() const { return currentState; }

	void SetSpeedUp(float s) { speedUp = s; }
	void SetBaseSpeed(float s) { baseSpeed = s; }

	float GetCurrentAnimationSeconds() const { return animationSeconds; }
	float GetAnimationSeconds(PlayerAnimationState state) const;

protected:
	// サブクラスでオーバーライド出来る
	virtual void OnStateChanged(StateEnum oldState, StateEnum newState) {}

	// モデルへのアクセス（派生クラスで実装すること）
	virtual std::shared_ptr<Model> GetModel() = 0;
	virtual const std::shared_ptr<Model> GetModel() const = 0;

	// アニメーション関連データ
	StateEnum currentState = static_cast<StateEnum>(-1);
	StateEnum previousState = static_cast<StateEnum>(-1);

	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	int animationIndex = -1;
	float animationSeconds = 0.0f;
	float oldAnimationSeconds = 0.0f;
	float animationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = 0.2f;
	bool isBlending = true;
	bool animationLoop = false;
	bool useRootMotion = false;
	bool useRootMotionEx = false;
	bool bakeTranslationY = true;

	// ヒットストップ用の変数
	bool hitStop = false;
	float hitStopLastSeconds = 0.0f;
	float hitStopSecondsLength = 0.3f;
	float cameraShakeRange = 0.03f;

	float baseSpeed = 1.0f;
	float speedUp = 0.0f;

	// 上半身と下半身のアニメーション用
	int overlayAnimationIndex = -1;       // 上半身アニメのインデックス
	float overlayAnimationSeconds = 0.0f; // 上半身アニメの再生時間
	bool overlayAnimationLoop = false;     
	bool isOverlayPlaying = false;        // 上半身アニメ再生中か


	DirectX::XMFLOAT4X4 worldTransform = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
};
