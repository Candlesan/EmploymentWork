#pragma once

#include<string>

class ActionBase;
class JudgmentBase;
class NodeBase;
class BehaviorData;
class Enemy;

//ビヘビアツリー
class BehaviorTree
{
public :
	//選択ルール
	enum class SelectRule
	{
		Non,              //末端ノード用
		Priority,         //優先順位
		Sequence,         //シーケンス
		SequentialLooping,//シーケンシャルルーピング
		Random            //ランダム
	};

public:
	BehaviorTree() :root(nullptr), owner(nullptr) {}
	BehaviorTree(Enemy* enemy) : root(nullptr), owner(enemy) {}
	~BehaviorTree();

	//実行ノード推論
	NodeBase* ActiveNodeInference(BehaviorData* data);
	//シーケンスノードから推論
	NodeBase* SequenceBase(NodeBase* sequenceNode, BehaviorData* data);
	//ノード追加
	void AddNode(std::string parentName, std::string entryName, int priority, SelectRule selectRule, JudgmentBase* judgment, ActionBase* action);

	//実行
	NodeBase* Run(NodeBase* actionNode, BehaviorData* data, float elapsedTime);

private:
	//ノード全削除
	void NodeAllClear(NodeBase* delNode);

private:
	//ルートノード
	NodeBase* root;
	Enemy* owner;
};
