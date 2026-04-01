#include "AnimationCharacter.h"
#include "AnimationStateManager.h"


// ステート変更
template<typename StateEnum>
void AnimationCharacter<StateEnum>::ChangeAnimationState(StateEnum newState, bool ignoreOverlay)
{
	if (currentState == newState) return;

	// オーバーレイ中はステート更新だけスキップ
	if (!ignoreOverlay && isOverlayPlaying) return;

	// ステート設定を取得
	const AnimationConfig* config = AnimationStateManager<StateEnum>::Instance().GetConfig(newState);
	if (!config) return;

	// モデルを取得
	std::shared_ptr<Model> model = GetModel();
	if (!model) return;

	// アニメーションインデックス取得
	int newAnimationIndex = model->GetAnimationIndex(config->animationName.c_str());
	if (newAnimationIndex < 0) return;

	// コールバック
	OnStateChanged(currentState, newState);

	// ステ-ト更新
	previousState = currentState;
	currentState = newState;

	// アニメーション切り替え
	if (animationIndex != newAnimationIndex)
	{
		oldNodePoses = nodePoses;
		animationIndex = newAnimationIndex;
		animationSeconds = 0.0f;
		oldAnimationSeconds = 0.0f;
		animationBlendSeconds = 0.0f;
		isBlending = true;
	}

	//アニメーションのリソースが同じでも、ステートが変われば「ループ設定」などは更新
	animationLoop = config->loop;
	useRootMotion = config->useRootMotion;
	useRootMotionEx = config->useRootMotionEx;
	animationBlendSecondsLength = config->blendTime;
}

// 上半身のアニメーション
template<typename StateEnum>
void AnimationCharacter<StateEnum>::StartOverlayAnimation(StateEnum newState)
{
	if (currentState == newState) return;

	// ステート設定を取得
	const AnimationConfig* config = AnimationStateManager<StateEnum>::Instance().GetConfig(newState);
	if (!config) return;

	// モデルを取得
	std::shared_ptr<Model> model = GetModel();
	if (!model) return;

	// アニメーションインデックス取得
	int newAnimationIndex = model->GetAnimationIndex(config->animationName.c_str());
	if (newAnimationIndex < 0) return;

	overlayAnimationIndex = newAnimationIndex;
	overlayAnimationSeconds = 0.0f;
	overlayAnimationLoop = config->loop;
	isOverlayPlaying = true;
}

// アニメーション更新
template<typename StateEnum>
void AnimationCharacter<StateEnum>::UpdateAnimation(float elapsedTime)
{
	if (animationIndex < 0) return;

	std::shared_ptr<Model> model = GetModel();
	if (!model) return;

	const Model::Animation& animation = model->GetAnimations().at(animationIndex);

	// 補間時間更新
	if (isBlending)
	{
		animationBlendSeconds += elapsedTime;
		if (animationBlendSeconds >= animationBlendSecondsLength)
		{
			isBlending = false;
			animationBlendSeconds = animationBlendSecondsLength;
		}
	}

	// 現在のアニメーションの姿勢を取得
	std::vector<Model::NodePose> currentNodePoses;
	model->ComputeAnimation(animationIndex, animationSeconds, currentNodePoses);

	// 補間処理
	if (isBlending && animationBlendSeconds < animationBlendSecondsLength)
	{
		float blendRate = animationBlendSeconds / animationBlendSecondsLength;
		model->BlendAnimations(oldNodePoses, currentNodePoses, blendRate, nodePoses);
	}
	else
	{
		nodePoses = currentNodePoses;
	}

	if (useRootMotion)
	{
		// ルートモーションノード番号取得
		const int rootMotionNodeIndex = model->GetNodeIndex(rootMotionNodeName.c_str()); // モデルに合わせて変えてくれ

		// 初回、前回、今回のルートモーションの姿勢を取得
		Model::NodePose beginPose, oldPose, newPose;
		model->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
		model->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
		model->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

		//ローカル移動値を算出
		DirectX::XMFLOAT3 localTranslation;
		if (oldAnimationSeconds > animationSeconds)
		{
			// ループ時処理
			Model::NodePose endPose;
			model->ComputeAnimation(animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);

			// ローカル移動値を算出
			localTranslation.x = (endPose.position.x - oldPose.position.x) +
				(newPose.position.x - beginPose.position.x);
			localTranslation.y = (endPose.position.y - oldPose.position.y) +
				(newPose.position.y - beginPose.position.y);
			localTranslation.z = (endPose.position.z - oldPose.position.z) +
				(newPose.position.z - beginPose.position.z);
		}
		else
		{
			localTranslation.x = newPose.position.x - oldPose.position.x;
			localTranslation.y = newPose.position.y - oldPose.position.y;
			localTranslation.z = newPose.position.z - oldPose.position.z;
		}

		DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);

		//ルートモーションノードを初回の姿勢にする
		nodePoses[rootMotionNodeIndex].position = beginPose.position;

		//ワールド移動値を算出
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);
		DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, WorldTransform);
		DirectX::XMFLOAT3 worldTranslation;
		DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);

		//移動値を更新
		this->position.x += worldTranslation.x;
		this->position.y += worldTranslation.y;
		this->position.z += worldTranslation.z;
	}
	else if (useRootMotionEx)
	{
		// ルートモーションノード番号取得（モデルに応じて適切なノード名を指定）
		const int rootMotionNodeIndex = model->GetNodeIndex(rootMotionNodeName.c_str());
		if (rootMotionNodeIndex >= 0)
		{
			// 初回、前回、今回のルートモーションの姿勢を取得
			Model::NodePose beginPose, oldPose, newPose;
			model->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
			model->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
			model->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

			// ローカル移動値を算出
			DirectX::XMFLOAT3 localTranslation;
			if (oldAnimationSeconds > animationSeconds)
			{
				// ループ時処理
				Model::NodePose endPose;
				model->ComputeAnimation(animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);
				// ローカル移動値を算出
				localTranslation.x = (endPose.position.x - oldPose.position.x) +
					(newPose.position.x - beginPose.position.x);
				localTranslation.y = (endPose.position.y - oldPose.position.y) +
					(newPose.position.y - beginPose.position.y);
				localTranslation.z = (endPose.position.z - oldPose.position.z) +
					(newPose.position.z - beginPose.position.z);
			}
			else
			{
				localTranslation.x = newPose.position.x - oldPose.position.x;
				localTranslation.y = newPose.position.y - oldPose.position.y;
				localTranslation.z = newPose.position.z - oldPose.position.z;
			}

			// グローバル移動値を算出
			Model::Node& rootMotionNode = model->GetNodes().at(rootMotionNodeIndex);
			DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);
			DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&rootMotionNode.parent->globalTransform);
			DirectX::XMVECTOR GlobalTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, ParentGlobalTransform);

			if (bakeTranslationY)
			{
				// Y成分の移動値を抜く 
				GlobalTranslation = DirectX::XMVectorSetY(GlobalTranslation, 0);

				// 今回の姿勢のグローバル位置を算出 
				DirectX::XMVECTOR LocalPos = DirectX::XMLoadFloat3(&newPose.position);
				DirectX::XMVECTOR currentGlobalPos = DirectX::XMVector3Transform(LocalPos, ParentGlobalTransform);

				// XZ成分を削除 
				currentGlobalPos = DirectX::XMVectorSetX(currentGlobalPos, 0);
				currentGlobalPos = DirectX::XMVectorSetZ(currentGlobalPos, 0);

				// ローカル空間変換 
				DirectX::XMMATRIX invGlobalTransform = DirectX::XMMatrixInverse(nullptr, ParentGlobalTransform);
				LocalPos = DirectX::XMVector3Transform(currentGlobalPos, invGlobalTransform);

				// ルートモーションノードの位置を設定 
				DirectX::XMStoreFloat3(&nodePoses[rootMotionNodeIndex].position, LocalPos);
			}
			else
			{
				//ルートモーションノードを初回の姿勢にする
				nodePoses[rootMotionNodeIndex].position = beginPose.position;
			}

			// ワールド移動値を算出（キャラクターの位置に足す）
			DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&transform);
			DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(GlobalTranslation, WorldTransform);
			DirectX::XMFLOAT3 worldTranslation;
			DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);

			//移動値を更新
			this->position.x += worldTranslation.x;
			this->position.y += worldTranslation.y;
			this->position.z += worldTranslation.z;
		}
	}

	if (isOverlayPlaying)
	{
		// 上半身アニメのポーズを計算
		std::vector<Model::NodePose> overlayPoses;
		model->ComputeAnimation(overlayAnimationIndex, overlayAnimationSeconds, overlayPoses);

		// モデルのノード一覧を取得 
		const auto& nodes = model->GetNodes();

		// 上半身ノードだけ nodePoses を上書き
		for (int i = 0; i < (int)nodes.size(); i++)
		{
			if (IsUpperBodyNode(nodes[i], upperBodyNodeName.c_str()))
			{
				nodePoses[i] = overlayPoses[i];
			}
		}

		// 上半身アニメーションの時間を進める
		overlayAnimationSeconds += elapsedTime;

		// アニメーション終了判定
		const Model::Animation& overlayAnim = model->GetAnimations().at(overlayAnimationIndex);
		if (overlayAnimationSeconds >= overlayAnim.secondsLength)
		{
			if (overlayAnimationLoop)
			{
				overlayAnimationSeconds -= overlayAnim.secondsLength; // ループ
			}
			else
			{
				isOverlayPlaying = false;
				overlayAnimationSeconds = 0.0f;
			}
		}
	}

	// アニメーション時間更新
	oldAnimationSeconds = animationSeconds;
	animationSeconds += elapsedTime * baseSpeed;

	// ループ処理
	bool looped = false;
	if (animationSeconds > animation.secondsLength)
	{
		if (animationLoop)
		{
			animationSeconds -= animation.secondsLength;
			looped = true;
		}
		else
		{
			animationSeconds = animation.secondsLength;
		}
	}

	//姿勢更新
	model->SetNodePoses(nodePoses);
}

// アニメーション終了判定
template<typename StateEnum>
bool AnimationCharacter<StateEnum>::IsAnimationFinished() const
{
	if (animationIndex < 0) return false;

	std::shared_ptr<Model> model = const_cast<AnimationCharacter*>(this)->GetModel();
	if (!model) return false;

	const Model::Animation& animation = model->GetAnimations().at(animationIndex);
	return animationSeconds >= animation.secondsLength;
}

// 再生時間で判定する関数（特定の範囲内なら遷移OK）
template<typename StateEnum>
bool AnimationCharacter<StateEnum>::IsAnimationInTimeRange(float startSeconds, float endSeconds) const
{
	if (animationIndex < 0) return false;
	// 現在の再生時間がstartSeconds ～　endSecondsの間にあるかをチェック

	return (animationSeconds >= startSeconds && animationSeconds <= endSeconds);
}

// 再生時間で判定する関数（特定の範囲外なら遷移OK）
template<typename StateEnum>
bool AnimationCharacter<StateEnum>::IsAnimationOutTimeRange(float startSeconds, float endSeconds) const
{
	if (animationIndex < 0) return false;
	// 現在の再生時間がstartSeconds ～　endSecondsの範囲外をチェック
	return (animationSeconds <= startSeconds || animationSeconds >= endSeconds);
}

// 再生時間で判定する関数（特定の場所以降なら遷移OK）
template<typename StateEnum>
bool AnimationCharacter<StateEnum>::IsAnimationOutTimeRange(float StartTransition) const
{
	if (animationIndex < 0) return false;
	// 現在の再生時間がStartTransition以上かををチェック
	return (animationSeconds >= StartTransition);
}

// だんだんアニメーション遅くする関数
template<typename StateEnum>
void AnimationCharacter<StateEnum>::AnimationLerp(float StartSlow, float endSlow, float SlowSpeed)
{
	if (animationSeconds >= StartSlow)
	{
		// 線形補間（Lerp）の考え方で速度を計算
		// t は 0.0 (開始) から 1.0 (終了) になる
		float t = (animationSeconds - StartSlow) / (endSlow - StartSlow);
		if (t > 1.0f) t = 1.0f;

		// 通常速度 -> スローへ変化
		float newSpeed = 1.0f - (t * SlowSpeed);
		SetBaseSpeed(newSpeed);
	}
}

// このノードを起点に子ノードが上半身の子かを判断する関数
template<typename StateEnum>
bool AnimationCharacter<StateEnum>::IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName)
{
	const Model::Node* current = &node;
	while (current != nullptr)
	{
		if (current->name == rootNodeName) return true;
		current = current->parent; // 親をたどる
	}
	return false;
}

template<typename StateEnum>
float AnimationCharacter<StateEnum>::GetCurrentAnimationLength() const
{
	if (animationIndex < 0) return false;

	std::shared_ptr<Model> model = const_cast<AnimationCharacter*>(this)->GetModel();
	if (!model) return false;

	const Model::Animation& animation = model->GetAnimations().at(animationIndex);

	return animation.secondsLength;
}

template class AnimationCharacter<PlayerAnimationState>;
template class AnimationCharacter<EnemyAnimationState>;