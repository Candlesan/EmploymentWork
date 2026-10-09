#include "BehaviorTree.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/ActionDerived.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/NodeBase.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/JudgmentBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorData.h"

// デストラクタ
BehaviorTree::~BehaviorTree()
{
	NodeAllClear(root);
}

void BehaviorTree::AddNode(std::string parentName, std::string entryName, int priority, SelectRule selectRule, JudgmentBase* judgment, ActionBase* action, bool interruptible)
{
	if (parentName != "")
	{
		NodeBase* parentNode = root->SearchNode(parentName);

		if (parentNode != nullptr)
		{
			NodeBase* addNode = new NodeBase(entryName, parentNode, priority, selectRule, judgment, action, interruptible);
			parentNode->AddChild(addNode);
		}
	}
	else
	{
		if (root == nullptr)
		{
			root = new NodeBase(entryName, nullptr, priority, selectRule, judgment, action);
		}
	}
}

// 次に実行するノードを推論
NodeBase* BehaviorTree::ActiveNodeInference(BehaviorData* data)
{
	if (root == nullptr) return nullptr;

	// データをリセットして開始
	data->Init();
	return root->Inference(data);
}

// シーケンスノードからの推論開始
NodeBase* BehaviorTree::SequenceBack(NodeBase* sequenceNode, BehaviorData* data)
{
	return sequenceNode->Inference(data);
}

// ノード実行
NodeBase* BehaviorTree::Run(NodeBase* actionNode, BehaviorData* data, float elapsedTime)
{
	// ノード実行
	ActionBase::State state = actionNode->Run(elapsedTime);

	// 正常終了（ここを元々の1つにまとめる）
	if (state == ActionBase::State::Complete)
	{
		// シーケンスの途中（連続行動のノード）があるかを確認
		NodeBase* sequenceNode = data->PopSequenceNode();

		// 途中じゃない（次の行動が決まっていない）なら終了
		if (sequenceNode == nullptr)
		{
			return nullptr;
		}
		else {
			// 途中の連続行動があるならそこから始める
			return SequenceBack(sequenceNode, data);
		}
	}
	else if (state == ActionBase::State::Failed) // ※FailedかFailureかはActionBaseの定義に合わせてな
	{
		// 失敗は終了
		return nullptr;
	}

	// まだアニメーション中（State::Run）なら現状維持
	return actionNode;
}

// 登録されたノードをすべて削除する
void BehaviorTree::NodeAllClear(NodeBase* delNode)
{
	if (delNode == nullptr) return;

	size_t count = delNode->children.size();
	if (count > 0)
	{
		for (NodeBase* node : delNode->children)
		{
			NodeAllClear(node);
		}
		delete delNode;
	}
	else
	{
		delete delNode;
	}
}
