#include"Enemy.h"
#include"Graphics.h"

#include"BehaviorTree.h"
#include"BehaviorData.h"
#include"NodeBase.h"
#include"JudgmentDerive.h"
#include"ActionDerive.h"
#include"Mathf.h"

#include"imgui.h"

//コンストラクタ
Enemy::Enemy()
{
	//デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	//モデル読み込み
	enemy = std::make_shared<Model>(device, "Data/Model/unitychan/unitychan.glb");
	//スケーリング
	//scale = { 0.01f,0.01f,0.01f };

	position.z = 2;

	//ベヘビアツリー設定
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree(this);

	//親ノード
	aiTree->AddNode("", "Root", 0, BehaviorTree::SelectRule::Sequence, nullptr, nullptr);
	//中間ノード
	aiTree->AddNode("Root", "Scout", 3, BehaviorTree::SelectRule::Sequence, nullptr, nullptr);
	//末端ノード
	aiTree->AddNode("Scout", "Stop", 2, BehaviorTree::SelectRule::Non, nullptr, new StopAction(this));
	aiTree->AddNode("Scout", "Move", 1, BehaviorTree::SelectRule::Non, new MoveJudgment(this), new MoveAction(this));

}

//デストラクタ
Enemy::~Enemy()
{
	delete aiTree;
	delete behaviorData;
}

//更新
void Enemy::Update(float elapsedTime)
{
	//現在実行中のノードがなければ
	if (activeNode == nullptr)
	{
		//次に実行するノードを推論する
		activeNode = aiTree->ActiveNodeInference(behaviorData);
	}
	//実行するノードがあれば
	if (activeNode != nullptr)
	{
		//ツリーからノードを実行
		activeNode = aiTree->Run(activeNode, behaviorData, elapsedTime);
	}


	//アニメーション更新処理
	UpdateAnimations(elapsedTime);
	//速力処理更新
	UpdateVelocity(elapsedTime);
	//オブジェクト行列更新
	UpdateTransform();
	//モデル行列更新
	enemy->UpdateTransform(transform);
}

//描画
void Enemy::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, enemy);
}

//GUI描画
void Enemy::DrawGui()
{
	ImGui::Begin("Enemy Info");

	//位置を表示
	ImGui::Text("Position: X=%.2f, Y=%.2f, Z=%.2f", position.x, position.y, position.z);
	//目的地を表示
	ImGui::Text("Target:   X=%.2f, Y=%.2f, Z=%.2f", targetPosition.x, targetPosition.y, targetPosition.z);
	
	ImGui::End();
}

//アニメーション更新処理
void Enemy::UpdateAnimations(float elapsedTime)
{
	int newAnimationIndex = animationIndex;

	switch (animationState)
	{
	case AnimationState::Idle:
		animationLoop = true;
		useRootMotion = false;
		useRootMotionEx = false;
		newAnimationIndex = enemy->GetAnimationIndex("Idle");

		break;
	case AnimationState::Walk:
		animationLoop = true;
		useRootMotion = false;
		useRootMotionEx = false;
		newAnimationIndex = enemy->GetAnimationIndex("WalkForwardInPlace");
		
		break;
	}

	//アニメーション切り替え
	if (animationIndex != newAnimationIndex)
	{
		//前のアニメーション姿勢を保持
		oldNodePoses = nodePoses;

		//新しいアニメーションに切り替え
		animationIndex = newAnimationIndex;
		animationSeconds = 0.0f;
		oldAnimationSeconds = 0.0f;

		//補間開始
		animationBlendSeconds = 0.0f;
		isBlending = true;
	}

	//補間更新
	if (isBlending)
	{
		animationBlendSeconds += elapsedTime;
		if (animationBlendSeconds >= animationBlendSecondsLength)
		{
			isBlending = false;
			animationBlendSeconds = animationBlendSecondsLength;
		}
	}

	if (animationIndex >= 0)
	{
		const Model::Animation& animation = enemy->GetAnimations().at(animationIndex);

		//現在のアニメーション姿勢を取得
		std::vector<Model::NodePose>currentNodePoses;
		enemy->ComputeAnimation(animationIndex, animationSeconds, currentNodePoses);

		//補間処理
		if (isBlending && animationBlendSeconds < animationBlendSecondsLength)
		{
			//補間率を計算
			float blendRate = animationBlendSeconds / animationBlendSecondsLength;
			//前のアニメーションと現在のアニメーションを補間
			enemy->BlendAnimations(oldNodePoses, currentNodePoses, blendRate, nodePoses);
		}
		else
		{
			//補間なしで現在アニメーション姿勢を使用
			nodePoses = currentNodePoses;
		}

		oldAnimationSeconds = animationSeconds;
		animationSeconds += elapsedTime;
		
		if (animationSeconds > animation.secondsLength)
		{
			if (animationLoop)
				animationSeconds -= animation.secondsLength;
			else
				animationSeconds = animation.secondsLength;
		}
		
		enemy->SetNodePoses(nodePoses);
	}
}
//アニメーションが終了したか
bool Enemy::FinshedAnimation()
{
	if (animationIndex > -1)
	{
		const Model::Animation& animation = enemy->GetAnimations().at(animationIndex);
	
		if (animationSeconds >= animation.secondsLength)
		{
			return true;
		}
	}
	return false;
}
//アニメーションステートセッター
void Enemy::SetAnimationState(AnimationState SetState)
{
	if (animationState != SetState)
	{
		animationState = SetState;
	}
}

//目的地ランダム設定
void Enemy::SetRandomTargetPosition()
{
	float theta = Mathf::RandomRange(-DirectX::XM_PI, DirectX::XM_PI);
	float range = Mathf::RandomRange(0.0f, targetRange);
	targetPosition.x = position.x + sinf(theta) * range;
	targetPosition.y = position.y;
	targetPosition.z = position.z + cosf(theta) * range;
}

//目的地へ移動
void Enemy::MoveToTarget(float elapsedTime, float speedRate)
{
	//ターゲット方向の進行ベクトル算出
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float dist = sqrtf(vx * vx + vz * vz);
	vx /= dist;
	vz /= dist;

	//移動処理
	Move(vx, vz, moveSpeed * speedRate);
	//回転処理
	Turn(elapsedTime, vx, vz, turnSpeed * speedRate);
}
