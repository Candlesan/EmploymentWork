#pragma once
#include "System/Resource/Model/Model.h"
#include <memory>
#include <vector>

class ShapeRenderer; // 前方宣言

class PhysicsBone
{
public:
	void SetUp(std::shared_ptr<Model> model, const char* rootBoneName);
	void Update(float elapsedTime, const DirectX::XMFLOAT4X4& WorldMatrix);
	void DrawGui();
	void RenderDebug(ShapeRenderer* renderer);
private:
	struct PhysicsNode
	{
		Model::Node* node; // モデルのノード
		DirectX::XMFLOAT3 oldWorldPosition; // 前フレームの位置
	};

	void AddNodeRecursive(Model::Node* node);

	std::vector<PhysicsNode> physicsNodes; // 揺れもの対象ノードのリスト
	float gravity = 1.0f; // 重力
	float damping = 0.9; // 減速係数
	bool showBone = false; // ボーンの可視化
};

