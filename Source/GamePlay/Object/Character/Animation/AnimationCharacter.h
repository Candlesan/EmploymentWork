#pragma once
#include "GamePlay/Object/Character/Character.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>


// アニメーション機能を持つキャラクター基底クラス
class AnimationCharacter : public Character
{
public:
	AnimationCharacter() = default;
	virtual ~AnimationCharacter() = default;

	// ステート変更
	void ChangeAnimationState(const std::string& newState, bool ignoreOverlay = false);

	// 上半身のアニメーション
	void StartOverlayAnimation(const std::string& newState);

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
	std::string GetCurrentState() const { return currentState; }

	void SetSpeedUp(float s) { speedUp = s; }
	void SetBaseSpeed(float s) { baseSpeed = s; }

	// 現在の再生時間を取得
	float GetCurrentAnimationSeconds() const { return animationSeconds; }
	float GetCurrentAnimationLength() const;

	// JSONファイルパスを渡して、中のノード設定を読み込む関数
	void LoadAnimationData(const std::string& jsonPath);

	// 指定したステート名の設定を取得する関数
	const AnimationConfig* GetAnimationConfig(const std::string& stateName) const;
protected:
	// サブクラスでオーバーライド出来る
	virtual void OnStateChanged(const std::string& oldState, const std::string& newState) {}

	// モデルへのアクセス（派生クラスで実装すること）
	virtual std::shared_ptr<Model> GetModel() = 0;
	virtual const std::shared_ptr<Model> GetModel() const = 0;

	// アニメーション関連データ
	std::string currentState = "";
	std::string previousState = "";

	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	std::string rootMotionNodeName = "root";      // ルートモーションのノード名
	std::string upperBodyNodeName = "spine_01";  // 上半身の起点ノード名

	std::unordered_map<std::string, AnimationConfig> stateConfigs;

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