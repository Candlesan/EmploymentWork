#pragma once

#include <vector>
#include <string>
#include "BehaviorTree.h"
#include "ActionBase.h"

class JudgmentBase;
class BehaviorData;

// メモリリーク調査用
#define debug_new new(_NORMAL_BLOCK,__FILE__,__LINE__)

// ノード
class NodeBase
{
public:
	// コンストラクタ
	NodeBase(std::string name, NodeBase* parent, int priority,
		BehaviorTree::SelectRule selectRule, JudgmentBase* judgment, ActionBase* action):
		name(name), parent(parent), priority(priority),
		selectRule(selectRule), judgment(judgment), action(action), children(NULL)
	{
	}
	// デストラクタ
	~NodeBase();
	// 名前ゲッター
	std::string GetName() { return name; }
	// 優先順位ゲッター
	int GetPriority() { return priority; }
	// 子ノード追加
	void AddChild(NodeBase* child) { children.push_back(child); }
	// 行動データを持っているか
	bool HasAction() { return action != nullptr ? true : false; }
	// 実行可否判定
	bool Judgment();
	// 優先順位選択
	NodeBase* SelectPriority(std::vector<NodeBase*>* list);
	// ランダム選択
	NodeBase* SelectRandom(std::vector<NodeBase*>* list);
	// シーケンス選択
	NodeBase* SelectSequence(std::vector<NodeBase*>* list, BehaviorData* data);
	// ノード検索
	NodeBase* SearchNode(std::string searchName);
	// ノード推論
	NodeBase* Inference(BehaviorData* data);
	// 実行
	ActionBase::State Run(float elapsedTime);
	std::vector<NodeBase*> children; // 子ノード
protected:
	std::string				 name;			// 名前
	BehaviorTree::SelectRule selectRule;	// 選択ルール
	JudgmentBase*			 judgment;		// 判定クラス
	ActionBase*				 action;		// 実行クラス
	unsigned int			 priority;		// 優先実行
	NodeBase*				 parent;		// 親ノード
};
