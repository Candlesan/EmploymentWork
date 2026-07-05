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

	// ブレンド用の記憶
	bool isBlending = false;
	float blendSeconds = 0.0f;
	float blendSecondsLength = 0.2f;
	std::vector<Model::NodePose> oldPoses; // 1つ前のアニメーションの姿勢

	// ルートモーション用の記憶
	float oldSeconds = 0.0f;
	bool useRootMotion = false;
	bool useRootMotionEx = false;
	bool bakeTranslationY = false;
};

// アニメーション専門のクラス
class Animator
{
public:
	void Initialize(std::shared_ptr<Model> model) { targetModel = model; }

	void PlayBase(int animIndex, bool loop, float blendTime, bool rootMotion, bool rootMotionEx);
	void PlayUpper(int animIndex, bool loop, const std::string& boneName, float blendTime);
	
	DirectX::XMFLOAT3 Update(float elapsedTime, const DirectX::XMFLOAT4X4& worldTransform);

	bool IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName);

	float GetBaseCurrentSeconds() const { return baseLayer.currentSeconds; }
	float GetUpperCurrentSeconds() const { return upperLayer.currentSeconds; }
	float GetUpperAnimationLength() const;

	bool IsUpperPlaying() const { return upperLayer.animationIndex >= 0; }
	void StopUpper() { upperLayer.animationIndex = -1; upperLayer.currentSeconds = 0.0f; }
private:
	std::shared_ptr<Model> targetModel; // 動かす対象のモデル

	// レイヤー
	AnimLayer baseLayer; // 前進
	AnimLayer upperLayer; // 上半身

	std::vector<Model::NodePose> finalPoses; // 最終的に完成したポーズ
};
