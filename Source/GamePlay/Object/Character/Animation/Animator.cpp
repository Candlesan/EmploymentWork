#include "Animator.h"


// ベースレイヤー
void Animator::PlayBase(int animIndex, bool loop, float blendTime, bool rootMotion, bool rootMotionEx)
{
	// アニメが違うものに切り替わる瞬間なら、今の全身ポーズを退避してブレンド開始
	if (baseLayer.animationIndex != animIndex && !finalPoses.empty())
	{
		baseLayer.oldPoses = finalPoses;
		baseLayer.isBlending = true;
		baseLayer.blendSeconds = 0.0f;
	}

	baseLayer.animationIndex = animIndex;
	baseLayer.isLoop = loop;
	baseLayer.blendSecondsLength = blendTime; // ブレンド時間を受け取る
	baseLayer.useRootMotion = rootMotion;
	baseLayer.useRootMotionEx = rootMotionEx;
	baseLayer.currentSeconds = 0.0f;
	baseLayer.oldSeconds = 0.0f;

	baseLayer.mixAnimationIndex = -1;
}

// 上半身のレイヤー
void Animator::PlayUpper(int animIndex, bool loop, const std::string& boneName, float blendTime)
{
	if (upperLayer.animationIndex != animIndex && !finalPoses.empty())
	{
		upperLayer.oldPoses = finalPoses;
		upperLayer.isBlending = true;
		upperLayer.blendSeconds = 0.0f;
	}

	upperLayer.animationIndex = animIndex;
	upperLayer.isLoop = loop;
	upperLayer.blendSecondsLength = blendTime;
	upperLayer.currentSeconds = 0.0f;
	upperLayer.rootBoneName = boneName;

	upperLayer.mixAnimationIndex = -1;
}

// 加算ブレンド
void Animator::PlayBaseMix(
	int animIndexA, int animIndexB,
	float mixRate,
	bool loop, float blendTime, bool rootMotion, bool rootMotionEx)
{
	// アニメが違うものに切り替わる瞬間なら、今の全身ポーズを退避してブレンド開始
	if (baseLayer.animationIndex != animIndexA && !finalPoses.empty())
	{
		baseLayer.oldPoses = finalPoses;
		baseLayer.isBlending = true;
		baseLayer.blendSeconds = 0.0f;
	}

	baseLayer.animationIndex = animIndexA;		// ベースのアニメーション
	baseLayer.mixAnimationIndex = animIndexB;	// 混ぜるアニメーション
	baseLayer.mixRate = mixRate;				// 混ぜる割合

	baseLayer.isLoop = loop;
	baseLayer.blendSecondsLength = blendTime;
	baseLayer.useRootMotion = rootMotion;
	baseLayer.useRootMotionEx = rootMotionEx;

	baseLayer.currentSeconds = 0.0f;
	baseLayer.mixcurrentSeconds = 0.0f; // Bの時間も0から始める
	baseLayer.oldSeconds = 0.0f;
}

void Animator::PlayUpperMix(int animIndexA, int animIndexB, float mixRate, bool loop, const std::string& boneName, float blendTime) {
	if (upperLayer.animationIndex != animIndexA && !finalPoses.empty())
	{
		upperLayer.oldPoses = finalPoses;
		upperLayer.isBlending = true;
		upperLayer.blendSeconds = 0.0f;
	}

	upperLayer.animationIndex = animIndexA;
	upperLayer.isLoop = loop;
	upperLayer.blendSecondsLength = blendTime;
	upperLayer.currentSeconds = 0.0f;
	upperLayer.rootBoneName = boneName;
}

// 更新処理
DirectX::XMFLOAT3 Animator::Update(float elapsedTime, const DirectX::XMFLOAT4X4& worldTransform)
{
	if (!targetModel) return{ 0, 0, 0 };

	DirectX::XMFLOAT3 rootMotionDelta = { 0.0f, 0.0f, 0.0f };

	// ベースとなるアニメーションを計算する
	if (baseLayer.animationIndex >= 0)
	{
		const Model::Animation& animation = targetModel->GetAnimations().at(baseLayer.animationIndex);

		if (baseLayer.isBlending)
		{
			baseLayer.blendSeconds += elapsedTime;
			if (baseLayer.blendSeconds >= baseLayer.blendSecondsLength)
			{
				baseLayer.isBlending = false;
			}
		}

		std::vector<Model::NodePose> currentNodePoses;
		// Mix用のアニメーションがあったら
		if (baseLayer.mixAnimationIndex >= 0)
		{
			std::vector<Model::NodePose> poseA;
			std::vector<Model::NodePose> poseB;

			// 両方のポーズを計算する
			targetModel->ComputeAnimation(baseLayer.animationIndex, baseLayer.currentSeconds, poseA);
			targetModel->ComputeAnimation(baseLayer.mixAnimationIndex, baseLayer.mixcurrentSeconds, poseB);

			targetModel->AccumulateAnimation(poseA, poseB, baseLayer.mixRate, currentNodePoses);
		}
		else
		{
			targetModel->ComputeAnimation(baseLayer.animationIndex, baseLayer.currentSeconds, currentNodePoses);
		}

		if (baseLayer.isBlending && baseLayer.blendSeconds < baseLayer.blendSecondsLength)
		{
			float rate = baseLayer.blendSeconds / baseLayer.blendSecondsLength;
			targetModel->BlendAnimations(baseLayer.oldPoses, currentNodePoses, rate, finalPoses);
		}
		else
		{
			finalPoses = currentNodePoses;
		}

		baseLayer.oldSeconds = baseLayer.currentSeconds;
		baseLayer.mixcurrentSeconds += elapsedTime;
		baseLayer.currentSeconds += elapsedTime;
		if (baseLayer.currentSeconds >= animation.secondsLength)
		{
			if (baseLayer.isLoop) {
				baseLayer.currentSeconds -= animation.secondsLength;
			}
			else {
				baseLayer.currentSeconds = animation.secondsLength;
			}
		}

		// ルートモーションの計算
		if (baseLayer.useRootMotion)
		{
			const int rootMotionNodeIndex = targetModel->GetNodeIndex("root");
			Model::NodePose beginPose, oldPose, newPose;
			targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, 0, beginPose);
			targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, baseLayer.oldSeconds, oldPose);
			targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, baseLayer.currentSeconds, newPose);

			DirectX::XMFLOAT3 localTranslation;
			if (baseLayer.oldSeconds > baseLayer.currentSeconds)
			{
				Model::NodePose endPose;
				targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);
				localTranslation.x = (endPose.position.x - oldPose.position.x) + (newPose.position.x - beginPose.position.x);
				localTranslation.y = (endPose.position.y - oldPose.position.y) + (newPose.position.y - beginPose.position.y);
				localTranslation.z = (endPose.position.z - oldPose.position.z) + (newPose.position.z - beginPose.position.z);
			}
			else
			{
				localTranslation.x = newPose.position.x - oldPose.position.x;
				localTranslation.y = newPose.position.y - oldPose.position.y;
				localTranslation.z = newPose.position.z - oldPose.position.z;
			}

			DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);
			finalPoses[rootMotionNodeIndex].position = beginPose.position;

			DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);
			DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, WorldTransform);
			DirectX::XMFLOAT3 worldTranslation;
			DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);

			rootMotionDelta.x += worldTranslation.x;
			rootMotionDelta.y += worldTranslation.y;
			rootMotionDelta.z += worldTranslation.z;
		}
		else if (baseLayer.useRootMotionEx)
		{
			const int rootMotionNodeIndex = targetModel->GetNodeIndex("root");
			if (rootMotionNodeIndex >= 0)
			{
				Model::NodePose beginPose, oldPose, newPose;
				targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, 0, beginPose);
				targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, baseLayer.oldSeconds, oldPose);
				targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, baseLayer.currentSeconds, newPose);

				DirectX::XMFLOAT3 localTranslation;
				if (baseLayer.oldSeconds > baseLayer.currentSeconds)
				{
					Model::NodePose endPose;
					targetModel->ComputeAnimation(baseLayer.animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);
					localTranslation.x = (endPose.position.x - oldPose.position.x) + (newPose.position.x - beginPose.position.x);
					localTranslation.y = (endPose.position.y - oldPose.position.y) + (newPose.position.y - beginPose.position.y);
					localTranslation.z = (endPose.position.z - oldPose.position.z) + (newPose.position.z - beginPose.position.z);
				}
				else
				{
					localTranslation.x = newPose.position.x - oldPose.position.x;
					localTranslation.y = newPose.position.y - oldPose.position.y;
					localTranslation.z = newPose.position.z - oldPose.position.z;
				}

				Model::Node& rootMotionNode = targetModel->GetNodes().at(rootMotionNodeIndex);
				DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);
				DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&rootMotionNode.parent->globalTransform);
				DirectX::XMVECTOR GlobalTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, ParentGlobalTransform);

				if (baseLayer.bakeTranslationY)
				{
					GlobalTranslation = DirectX::XMVectorSetY(GlobalTranslation, 0);
					DirectX::XMVECTOR LocalPos = DirectX::XMLoadFloat3(&newPose.position);
					DirectX::XMVECTOR currentGlobalPos = DirectX::XMVector3Transform(LocalPos, ParentGlobalTransform);
					currentGlobalPos = DirectX::XMVectorSetX(currentGlobalPos, 0);
					currentGlobalPos = DirectX::XMVectorSetZ(currentGlobalPos, 0);
					DirectX::XMMATRIX invGlobalTransform = DirectX::XMMatrixInverse(nullptr, ParentGlobalTransform);
					LocalPos = DirectX::XMVector3Transform(currentGlobalPos, invGlobalTransform);
					DirectX::XMStoreFloat3(&finalPoses[rootMotionNodeIndex].position, LocalPos);
				}
				else
				{
					finalPoses[rootMotionNodeIndex].position = beginPose.position;
				}

				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);
				DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(GlobalTranslation, WorldTransform);
				DirectX::XMFLOAT3 worldTranslation;
				DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);

				rootMotionDelta.x += worldTranslation.x;
				rootMotionDelta.y += worldTranslation.y;
				rootMotionDelta.z += worldTranslation.z;
			}
		}
	}

	// 上半身のポーズを計算する
	if (upperLayer.animationIndex >= 0)
	{
		// upperLayer から情報を取得するように完全修正
		const Model::Animation& upperAnim = targetModel->GetAnimations().at(upperLayer.animationIndex);
		std::vector<Model::NodePose> upperPoses;

		if (upperLayer.isBlending)
		{
			upperLayer.blendSeconds += elapsedTime;
			if (upperLayer.blendSeconds >= upperLayer.blendSecondsLength)
			{
				upperLayer.isBlending = false;
			}
		}

		// Mix用のアニメーションがあったら
		if (baseLayer.mixAnimationIndex >= 0)
		{
			std::vector<Model::NodePose> poseA;
			std::vector<Model::NodePose> poseB;

			// 両方のポーズを計算する
			targetModel->ComputeAnimation(upperLayer.animationIndex, upperLayer.currentSeconds, poseA);
			targetModel->ComputeAnimation(upperLayer.mixAnimationIndex, upperLayer.mixcurrentSeconds, poseB);

			targetModel->AccumulateAnimation(poseA, poseB, upperLayer.mixRate, upperPoses);
		}
		else
		{
			targetModel->ComputeAnimation(upperLayer.animationIndex, upperLayer.currentSeconds, upperPoses);
		}

		const auto& nodes = targetModel->GetNodes();
		std::vector<Model::NodePose> finalUpperPoses; // 上半身専用の一時的な箱

		// 一時的な箱（finalUpperPoses）に正しくブレンド出力する
		if (upperLayer.isBlending && upperLayer.blendSeconds < upperLayer.blendSecondsLength)
		{
			float rate = upperLayer.blendSeconds / upperLayer.blendSecondsLength;
			targetModel->BlendAnimations(upperLayer.oldPoses, upperPoses, rate, finalUpperPoses);
		}
		else
		{
			finalUpperPoses = upperPoses;
		}

		// 上半身のノードだけ finalPoses を上書き
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (IsUpperBodyNode(nodes[i], upperLayer.rootBoneName))
			{
				finalPoses[i] = finalUpperPoses[i]; // ここで初めて全身ポーズの骨を上書き
			}
		}

		upperLayer.oldSeconds = upperLayer.currentSeconds;
		upperLayer.currentSeconds += elapsedTime;
		upperLayer.mixcurrentSeconds += elapsedTime;

		if (upperLayer.currentSeconds >= upperAnim.secondsLength)
		{
			if (upperLayer.isLoop)
			{
				upperLayer.currentSeconds -= upperAnim.secondsLength;
			}
			else
			{
				upperLayer.animationIndex = -1;
				upperLayer.currentSeconds = 0.0f;
			}
		}
	}

	// 姿勢更新
	if (!finalPoses.empty())
	{
		targetModel->SetNodePoses(finalPoses);
	}

	return rootMotionDelta;
}

// このノードを起点に子ノードが上半身の子かを判断する関数
bool Animator::IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName)
{
	const Model::Node* current = &node;
	while (current != nullptr)
	{
		if (current->name == rootNodeName) return true;
		current = current->parent; // 親をたどる
	}
	return false;
}

// 上半身のアニメーション再生時間を取得
float Animator::GetUpperAnimationLength() const
{
	if (upperLayer.animationIndex < 0) return false;

	const Model::Animation& animation = targetModel->GetAnimations().at(upperLayer.animationIndex);

	return animation.secondsLength;
}

// 
void Animator::StopUpper()
{
	upperLayer.animationIndex = -1;
	upperLayer.currentSeconds = 0.0f;
	upperLayer.mixAnimationIndex = -1;
}