#pragma once
#include "System/UI/AnimationNodeEditor/AnimationTranstion.h"
#include <imgui_node_editor.h>
#include "json.hpp"
#include <fstream>
#include <string>
namespace ed = ax::NodeEditor;

struct AnimNode {
	ed::NodeId nodeId;
	int animState; // PlayerAnimationState
	ImVec2 position; // ノードの位置
	ed::PinId pinOut; // 遷移の出口
	ed::PinId pinIn; // 遷移の入口
};

struct AnimLink {
	ed::LinkId linkId;
	ed::PinId startPin; // fromノードpinOut
	ed::PinId endPin; // toノードのpinIn
	AnimationTransition transition; // この矢印の遷移条件
	ImVec4 color = ImVec4(1, 1, 1, 1);
};

class AnimationTransitionGraph
{
public:
	std::vector<AnimNode> nodes;
	std::vector<AnimLink> links;
	std::string graphName = "NewGraph"; // グラフの名前

	// ===== 基本機能 ======
	void AddNode(int animState, ImVec2 pos); // ノードの追加
	void AddLink(ed::PinId from, ed::PinId to); // 矢印の追加
	void RemoveNode(ed::NodeId id); // ノードの削除
	void RemoveLink(ed::LinkId id); // 矢印の削除

	void Save(const std::string& path); // Jsonファイルに保存する
	void Load(const std::string& path); // Jsonファイルを読み込む
	// =====================

	// ランタイム評価：現在のステートから次ステートを返す
	int EvaluateTransitions(int currentState, const TransitionContext& conditions);

	// 自動整列
	void AutoLayout();

	// 新しいグラフのセットアップ
	void InitializeAsNew(const std::string& name);

	// ゲッター
	const AnimationTransition* GetTransition(int fromState, int toState);
private:
	int nextId = 1;
	int NextId() { return nextId++; }
	bool EvaluateConditions(const std::vector<TransitionCondition>& conditions,
		const TransitionContext& ctx);
};
