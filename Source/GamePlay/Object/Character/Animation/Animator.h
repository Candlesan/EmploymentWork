#pragma once
#include "System/Resource/Model/Model.h"

#include <string>
#include <vector>
#include <memory>

// レイヤーをまとめる構造体
struct AnimLayer {
	int animationIndex = -1;
	float currentSeconds = 0.0f;
	float blendWeight = 1.0f;
	bool isLoop = false;
	std::string rootBoneName = "";

	// ブレンド用の変数
	bool isBlending = false;
	float blendSeconds = 0.0f;
	float blendSecondsLength = 0.2f;
	std::vector<Model::NodePose> oldPoses; // 1つ前のアニメーションの姿勢

	// ルートモーション用の変数
	float oldSeconds = 0.0f;
	bool useRootMotion = false;
	bool useRootMotionEx = false;
	bool bakeTranslationY = false;

	// 加算ブレンド用の変数
	int mixAnimationIndex = -1;
	float mixcurrentSeconds = 0.0f;
	float mixRate = 0.0f; // 混ぜる割合

	bool isFadeingOut = false;
	float fadeOutSeconds = 0.0f;
	float fadeOutLength = 0.2;
	std::vector<Model::NodePose> fadeOutOldPose;
};

// アニメーション専門のクラス
class Animator
{
public:
	void Initialize(std::shared_ptr<Model> model) { targetModel = model; }

	// レイヤー各種設定
	void PlayBase(int animIndex, bool loop, float blendTime, bool rootMotion, bool rootMotionEx);
	void PlayUpper(int animIndex, bool loop, const std::string& boneName, float blendTime);
	
	// 加算ブレンド各レイヤー設定
	void PlayBaseMix(int animIndexA, int animIndexB, float mixRate,
		bool loop, float blendTime, bool rootMotion, bool rootMotionEx);
	void PlayUpperMix(int animIndexA, int animIndexB, float mixRate, bool loop, const std::string& boneName, float blendTime);
	DirectX::XMFLOAT3 Update(float elapsedTime, const DirectX::XMFLOAT4X4& worldTransform);

	bool IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName);

	float GetBaseCurrentSeconds() const { return baseLayer.currentSeconds; }
	float GetUpperCurrentSeconds() const { return upperLayer.currentSeconds; }
	float GetUpperAnimationLength() const;

	bool IsUpperPlaying() const { return upperLayer.animationIndex >= 0; }
	void StopUpper();

	void SetBankAngle(float angle) { bankAngle = angle; }
private:
	std::shared_ptr<Model> targetModel; // 動かす対象のモデル

	// レイヤー
	AnimLayer baseLayer; // 前進
	AnimLayer upperLayer; // 上半身

	std::vector<Model::NodePose> finalPoses; // 最終的に完成したポーズ

	float bankAngle = 0.0f; // 目標の傾きを入れる変数
};
