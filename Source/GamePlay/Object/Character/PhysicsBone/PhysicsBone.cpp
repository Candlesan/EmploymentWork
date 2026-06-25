#include "PhysicsBone.h"
#include <imgui.h>
#include "System/Renderer/ShapeRenderer.h"


// 物理ボーン設定
void PhysicsBone::SetUp(std::shared_ptr<Model> model, const char* rootBoneName)
{
	// ルートボーンのノードを名前で探す
	for (auto& nodes : model->GetNodes())
	{
		if (nodes.name == rootBoneName)
		{
			// 見つかったノードから子ボーンを再帰的にたどってリストに追加
			AddNodeRecursive(&nodes);
		}
	}
}

// 更新処理
void PhysicsBone::Update(float elapsedTime, const DirectX::XMFLOAT4X4& WorldMatrix)
{
	const float gravity = this->gravity * elapsedTime;
	for (int i = 2; i < static_cast<int>(physicsNodes.size()); ++i)
	{
		// 各ボーン情報取得
		Model::Node* parent = physicsNodes[i - 2].node;
		Model::Node* bone   = physicsNodes[i - 1].node;
		Model::Node* child  = physicsNodes[i].node;

		// ボーンの回転制御でロープ表現をする物理処理
		{
			DirectX::XMMATRIX ChildWorldTransform = DirectX::XMLoadFloat4x4(&child->worldTransform);
			DirectX::XMVECTOR ChildWorldPosition = ChildWorldTransform.r[3];
			DirectX::XMVECTOR ChildOldWorldPosition = DirectX::XMLoadFloat3(&physicsNodes[i].oldWorldPosition);
			DirectX::XMStoreFloat3(&physicsNodes[i].oldWorldPosition, ChildWorldPosition);

			// 前回位置から今回位置の差分と重力による速力を算出する
			DirectX::XMVECTOR Gravity = DirectX::XMVectorSet(0, -gravity, 0, 0);
			DirectX::XMVECTOR Velocity = DirectX::XMVectorSubtract(ChildWorldPosition, ChildOldWorldPosition);
			DirectX::XMVECTOR Length = DirectX::XMVector3Length(Velocity);
			Velocity = DirectX::XMVectorScale(Velocity, damping);
			Velocity = DirectX::XMVectorAdd(Velocity, Gravity);

			// 子ボーンの位置を動かす
			ChildWorldPosition = DirectX::XMVectorAdd(ChildWorldPosition, Velocity);

			// 現在のボーンのローカル空間方向を求める
			DirectX::XMVECTOR ChildLocalPosition = DirectX::XMLoadFloat3(&child->position);
			DirectX::XMVECTOR LocalDirection = DirectX::XMVector3Normalize(ChildLocalPosition);

			// 最終的なボーンのローカル空間方向を決める
			DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&bone->worldTransform);
			DirectX::XMMATRIX InverceWorldTransform = DirectX::XMMatrixInverse(nullptr, WorldTransform);

			// XMVector3Transformは行列変換する関数
			DirectX::XMVECTOR LocalTargetDirection = DirectX::XMVector3Transform(ChildWorldPosition,
				InverceWorldTransform);
			LocalTargetDirection = DirectX::XMVector3Normalize(LocalTargetDirection);

			// 回転角度を算出
			DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(LocalDirection, LocalTargetDirection);
			float angle = acosf(std::clamp(DirectX::XMVectorGetX(Dot), -1.0f, 1.0f)); // 角度 = acos(dot)だから
			if (angle > FLT_EPSILON)
			{
				// ローカル回転軸を算出
				DirectX::XMVECTOR LocalAxis = DirectX::XMVector3Cross(LocalDirection, LocalTargetDirection);
				LocalAxis = DirectX::XMVector3Normalize(LocalAxis);

				// ローカル回転処理
				DirectX::XMVECTOR LocalRotationAxis = DirectX::XMQuaternionRotationAxis(LocalAxis, angle);
				DirectX::XMVECTOR LocalRotation = DirectX::XMLoadFloat4(&bone->rotation);
				LocalRotation = DirectX::XMQuaternionMultiply(LocalRotationAxis, LocalRotation);
				DirectX::XMStoreFloat4(&bone->rotation, LocalRotation);
			}

			// ワールド行列更新
			DirectX::XMMATRIX LocalPositionTransfrom = DirectX::XMMatrixTranslation(bone->position.x,
				bone->position.y, bone->position.z);
			DirectX::XMMATRIX LocalRotationTransform =
				DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&bone->rotation));
			DirectX::XMMATRIX LocalTransform = DirectX::XMMatrixMultiply(LocalRotationTransform,
				LocalPositionTransfrom);
			DirectX::XMMATRIX ParentWorldTransform = DirectX::XMLoadFloat4x4(&parent->worldTransform);
			WorldTransform = DirectX::XMMatrixMultiply(LocalTransform, ParentWorldTransform);
			DirectX::XMStoreFloat4x4(&bone->worldTransform, WorldTransform);

			// 子ワールド行列更新
			DirectX::XMMATRIX ChildLocalPositionTransform =
				DirectX::XMMatrixTranslation(child->position.x, child->position.y, child->position.z);
			DirectX::XMMATRIX ChildLocalRotationTransform =
				DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&child->rotation));
			DirectX::XMMATRIX ChildLocalTransform = DirectX::XMMatrixMultiply(ChildLocalRotationTransform,
				ChildLocalPositionTransform);
			ChildWorldTransform = DirectX::XMMatrixMultiply(ChildLocalTransform, WorldTransform);
			DirectX::XMStoreFloat4x4(&child->worldTransform, ChildWorldTransform);
		}
	}

	if (!physicsNodes.empty())
	{
		DirectX::XMMATRIX PlayerWorld = DirectX::XMLoadFloat4x4(&WorldMatrix);

		// リストの0番目（一番根元の親ボーン）に「下位の行列を全部リフレッシュして！」と頼む
		physicsNodes[0].node->UpdateWorldTransformsRecursive(PlayerWorld);
	}
}

void PhysicsBone::DrawGui()
{
	std::string headerName = "PhysicsBone (" + physicsNodes[0].node->name + ")";

	if (ImGui::CollapsingHeader(headerName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("gravity", &gravity, 0.01f);
		ImGui::DragFloat("damping", &damping, 0.01f);
		if (ImGui::Button("ShowPhysicsBone"))
		{
			showBone = !showBone;
		}
	}
}

void PhysicsBone::RenderDebug(ShapeRenderer* renderer)
{
	if (!showBone) return;

	for (int i = 1; i < static_cast<int>(physicsNodes.size()); ++i)
	{
		// 現在のボーン（骨の根元）と、子ボーン（骨の先端）を取得
		Model::Node* bone = physicsNodes[i - 1].node;
		Model::Node* child = physicsNodes[i].node;

		// 3. 骨の長さを計算するために、お互いのワールド座標を取り出す
		DirectX::XMVECTOR BonePos = DirectX::XMVectorSet(
			bone->worldTransform._41, bone->worldTransform._42, bone->worldTransform._43, 0.0f);
		DirectX::XMVECTOR ChildPos = DirectX::XMVectorSet(
			child->worldTransform._41, child->worldTransform._42, child->worldTransform._43, 0.0f);

		// 2点間の差分ベクトルを作って、その長さを測る
		DirectX::XMVECTOR FromBoneToChild = DirectX::XMVectorSubtract(ChildPos, BonePos);
		DirectX::XMVECTOR LengthVec = DirectX::XMVector3Length(FromBoneToChild);

		// XMVECTORからfloatに変換して長さを確定
		float length = DirectX::XMVectorGetX(LengthVec);

		// 4. ShapeRendererの DrawBone を呼び出す！
		// 骨の向きの行列には bone->worldTransform をそのまま使い、計算した長さを渡す
		DirectX::XMFLOAT4 boneColor = { 1.0f, 0.0f, 0.0f, 1.0f }; // 目立つように赤色にしてみる

		renderer->DrawBone(bone->worldTransform, length, boneColor);
	}
}

// 子ノードをたどる関数
void PhysicsBone::AddNodeRecursive(Model::Node* node)
{
	// このノードをリストに追加
	physicsNodes.push_back({ node, {} });

	// このノードに対して同じことをする
	for (Model::Node* child : node->children)
	{
		AddNodeRecursive(child);
	}

	// 初期位置を設定しておく
	for (auto& physicsNode : physicsNodes)
	{
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&physicsNode.node->worldTransform);
		DirectX::XMStoreFloat3(&physicsNode.oldWorldPosition, WorldTransform.r[3]);
	}
}
