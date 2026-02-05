#include"BehaviorTree.h"
#include"ActionBase.h"
#include"NodeBase.h"
#include"JudgmentBase.h"
#include"BehaviorData.h"

BehaviorTree::~BehaviorTree()
{
	//終了時ノードクリア
	NodeAllClear(root);
}

//ノード追加
void BehaviorTree::AddNode(std::string parentName, std::string entryName, int priority, SelectRule selectRule, JudgmentBase* judgment, ActionBase* action)
{

	if (parentName != "")
	{
		NodeBase* parentNode = root->SearchNode(parentName);
		if (parentNode != nullptr)
		{
			NodeBase* addNode = new NodeBase(entryName, parentNode, priority, selectRule, judgment, action);
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

//次に実行するノードを推論
NodeBase* BehaviorTree::ActiveNodeInference(BehaviorData* data)
{
	//データをリセットして開始
	data->Init();

	return root->Inference(data);
}

//シーケンスノードから推論開始
NodeBase* BehaviorTree::SequenceBase(NodeBase* sequenceNode, BehaviorData* data)
{
	return sequenceNode->Inference(data);
}

//ノード実行
NodeBase* BehaviorTree::Run(NodeBase* actionNode, BehaviorData* data, float elapsedTime)
{
	//ノード実行
	ActionBase::State state = actionNode->Run(elapsedTime);

	//正常終了
	if (state == ActionBase::State::Complete)
	{
		//シーケンス途中か判断
		NodeBase* sequenceNode = data->PopSequenceNode();

		//途中じゃないなら終了
		if (sequenceNode == nullptr)
		{
			return nullptr;
		}
		else
		{
			//途中ならそこから始める
			return SequenceBase(sequenceNode, data);
		}
	}
	else if (state == ActionBase::State::Failed)
	{
		//失敗は終了
		return nullptr;
	}

	//現状維持
	return actionNode;
}

//登録されたノードを全削除
void BehaviorTree::NodeAllClear(NodeBase* delNode)
{
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