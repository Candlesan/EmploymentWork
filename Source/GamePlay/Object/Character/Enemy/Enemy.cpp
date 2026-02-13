#include "Enemy.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"

#include <imgui.h>

// 初期化
void Enemy::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	enemy = std::make_shared<Model>(device, "Data/Model/Enemy/Map_Robot3.gltf");

	position = { 0, 0, 10 };
	angle = {0, 3, 0};
	weight = 100.0f;
	height = 1.0f;
	debugOffset = 0.8;
	health = 5;
	invincibleTimer = 0.0f;

	// アニメーション設定
	enemy->GetNodePoses(nodePoses);
	enemy->GetNodePoses(oldNodePoses);
	state = State::Idle;
}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	// アニメーション更新処理
	UpdateAnimations(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	enemy->UpdateTransform(transform);
}

// 描画処理
void Enemy::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, enemy);
}

// GUI描画
void Enemy::DrawGUI()
{
	ImGui::Begin("Enemy");

	ImGui::Text("Health: %f.0", health);

	// トランスフォーム情報
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Position", &position.x);

		DirectX::XMFLOAT3 a;
		a.x = DirectX::XMConvertToDegrees(angle.x);
		a.y = DirectX::XMConvertToDegrees(angle.y);
		a.z = DirectX::XMConvertToDegrees(angle.z);
		ImGui::DragFloat3("Angle", &angle.x);
		// 表示用に度数法に変換した後、再度ラジアンで戻す処理
		angle.x = DirectX::XMConvertToRadians(a.x);
		angle.y = DirectX::XMConvertToRadians(a.y);
		angle.z = DirectX::XMConvertToRadians(a.z);

		ImGui::DragFloat3("Scale", &scale.x);
	}

	// 当たり判定情報
	if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Radius:", &radius, 0.1f); // 当たり判定の半径
		ImGui::DragFloat("Height:", &height, 0.1f); // 当たり判定の高さ
		ImGui::DragFloat("Collision Transform Offset:", &debugOffset, 0.1f);
	}

	ImGui::End();
}

// デバックプリミティブ描画
void Enemy::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	DirectX::XMFLOAT4X4 capsuleTransform;
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + debugOffset, position.z);
	DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

	renderer->DrawCapsule(capsuleTransform, radius, height, { 1, 1, 0, 1 });
}

// アニメーション更新処理
void Enemy::UpdateAnimations(float elapsedTime)
{
	int newAnimationIndex = animationIndex;

	switch (state)
	{
	case Enemy::State::Idle:
		animationLoop = true;
		useRootMotion = false;
		useRootMotionEx = false;
		newAnimationIndex = enemy->GetAnimationIndex("Idle");

		break;
	}

	// アニメーション切り替え判定
	if (animationIndex != newAnimationIndex)
	{
		// 前のアニメーションの姿勢を保存
		oldNodePoses = nodePoses;

		// 新しいアニメーションに切り替え
		animationIndex = newAnimationIndex;
		animationSeconds = 0.0f;

		// 補間開始
		animationBlendSeconds = 0.0f;
		isBlending = true;
	}

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

	// アニメーション更新処理
	if (animationIndex >= 0)
	{
		const Model::Animation& animation = enemy->GetAnimations().at(animationIndex);

		// 現在のアニメーションの姿勢を取得
		std::vector<Model::NodePose> currentNodePoses;
		enemy->ComputeAnimation(animationIndex, animationSeconds, currentNodePoses);

		// 補間処理
		if (isBlending && animationBlendSeconds < animationBlendSecondsLength)
		{
			// 補間率を計算
			float blendRate = animationBlendSeconds / animationBlendSecondsLength;

			// 前のアニメーションと現在のアニメーションを補間
			enemy->BlendAnimations(oldNodePoses, currentNodePoses, blendRate, nodePoses);
		}
		else
		{
			// 補間なしで現在のアニメーション姿勢を使用
			nodePoses = currentNodePoses;
		}

		// ルートモーション処理
		if (useRootMotion)
		{
			{
				// ルートモーションノード番号取得
				const int rootMotionNodeIndex = enemy->GetNodeIndex("mixamorig:Hips");

				// 初回、前回、今回のルートモーションの姿勢を取得
				Model::NodePose beginPose, oldPose, newPose;
				enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
				enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
				enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

				DirectX::XMFLOAT3 localTranslation;

				if (oldAnimationSeconds > animationSeconds)
				{
					// ループ時処理
					Model::NodePose endPose;
					enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);
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
					// ローカル移動値
					localTranslation.x = newPose.position.x - oldPose.position.x;
					localTranslation.y = newPose.position.y - oldPose.position.y;
					localTranslation.z = newPose.position.z - oldPose.position.z;
				}

				DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);

				// ルートモーションを初回の姿勢にする
				nodePoses[rootMotionNodeIndex].position = beginPose.position;

				// ワールド移動値を算出
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&transform);
				DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, WorldTransform);
				DirectX::XMFLOAT3 worldTranslation;
				DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);


				// 位置を更新
				position.x += worldTranslation.x;
				position.y += worldTranslation.y;
				position.z += worldTranslation.z;
			}
		}

		// 腰骨に対応したルートモーション
		if (useRootMotionEx)
		{
			// ルートモーションノード番号取得（モデルに応じて適切なノード名を指定）
			const int rootMotionNodeIndex = enemy->GetNodeIndex("pelvis");
			if (rootMotionNodeIndex >= 0)
			{
				// 初回、前回、今回のルートモーションの姿勢を取得
				Model::NodePose beginPose, oldPose, newPose;
				enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
				enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
				enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

				// ローカル移動値を算出
				DirectX::XMFLOAT3 localTranslation;
				if (oldAnimationSeconds > animationSeconds)
				{
					// ループ時処理
					Model::NodePose endPose;
					enemy->ComputeAnimation(animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);
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
				Model::Node& rootMotionNode = enemy->GetNodes().at(rootMotionNodeIndex);
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
				position.x += worldTranslation.x;
				position.y += worldTranslation.y;
				position.z += worldTranslation.z;
			}
		}

		//アニメーション時間更新
		oldAnimationSeconds = animationSeconds;
		animationSeconds += elapsedTime;

		if (animationSeconds > animation.secondsLength)
		{
			if (animationLoop)
			{
				animationSeconds -= animation.secondsLength;
			}
			else
			{
				animationSeconds = animation.secondsLength;
			}
		}

		//姿勢更新
		enemy->SetNodePoses(nodePoses);
	}
}
