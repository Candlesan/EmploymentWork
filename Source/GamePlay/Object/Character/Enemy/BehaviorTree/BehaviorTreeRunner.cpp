#include "BehaviorTreeRunner.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorTree.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorData.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/NodeBase.h"

// コンストラクタ
BehaviorTreeRunner::BehaviorTreeRunner()
{
	tree = new BehaviorTree();
	data = new BehaviorData();
}

// デストラクタ
BehaviorTreeRunner::~BehaviorTreeRunner()
{
	delete tree;
	delete data;
}

// 更新処理
void BehaviorTreeRunner::Update(float elapsedTime)
{
	// 中断
	if (activeNode != nullptr && activeNode->IsInterruptible())
	{
		// 毎フレーム推論し直して、選ばれるノードが変わったら乗り換える
		NodeBase* next = tree->ActiveNodeInference(data);
		if (next != activeNode)
		{
			activeNode->Abort(); // 古い方を後始末する
			activeNode = next;
			if (activeNode != nullptr)
			{
				lastInferredNode = activeNode;
			}
		}
	}

	// 現在実行されているノードが無ければ推論
	if (activeNode == nullptr)
	{
		activeNode = tree->ActiveNodeInference(data);
		if (activeNode != nullptr)
		{
			lastInferredNode = activeNode;
		}
	}

	// 現在実行するノードがあれば実行
	if (activeNode != nullptr)
	{
		activeNode = tree->Run(activeNode, data, elapsedTime);
	}
}

// 途中中断
void BehaviorTreeRunner::Abort()
{
	if (activeNode != nullptr)
	{
		activeNode->Abort(); // 実行中のアクションの後始末（フラグを戻したりなど）
	}
	activeNode = nullptr;
	lastInferredNode = nullptr;
	data->Init(); // シーケンスの途中状態も捨てる
}

// ツリーをからにして作り直す
void BehaviorTreeRunner::Reset()
{
	Abort();
	delete tree;
	tree = new BehaviorTree();
}