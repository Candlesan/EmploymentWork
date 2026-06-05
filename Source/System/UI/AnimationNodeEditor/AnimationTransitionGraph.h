#pragma once
#include "System/UI/AnimationNodeEditor/AnimationTranstion.h"
#include <imgui_node_editor.h>
#include "json.hpp"
#include <fstream>
namespace ed = ax::NodeEditor;

// アニメーションの設定
struct AnimationConfig
{
	std::string animationName = "";	// アニメーション名
	bool loop = false;				// アニメーションをループするか
	bool useRootMotion = false;		// ルートモーションするか
	bool useRootMotionEx = false;	// 腰骨に対応したルートモーション
	float blendTime = 0.2f;			// 補間時間
};

// ノードのタイプ
enum class NodeType {
	Animation, // アニメーションを再生するためのノード
	SubGraph, // ダブルクリックしたときに階層に潜るための箱
};

struct AnimNode {
	ed::NodeId nodeId;
	std::string StateName; // 文字列に変える

	NodeType type = NodeType::Animation;
	std::string subGraphPath = "";

	AnimationConfig config; // アニメーション自体の設定

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
	void AddNode(const std::string& StateName, ImVec2 pos); // ノードの追加
	void AddLink(ed::PinId from, ed::PinId to); // 矢印の追加
	void RemoveNode(ed::NodeId id); // ノードの削除
	void RemoveLink(ed::LinkId id); // 矢印の削除

	void Save(const std::string& path); // Jsonファイルに保存する
	void Load(const std::string& path); // Jsonファイルを読み込む
	// =====================

	// ランタイム評価：現在のステートから次ステートを返す
	std::string EvaluateTransitions(const std::string& currentState, const TransitionContext& conditions);

	// 自動整列
	void AutoLayout();

	// 新しいグラフのセットアップ
	void InitializeAsNew(const std::string& name);

	int NextId() { return nextId++; }

	// ゲッター
	const AnimationTransition* GetTransition(const std::string& fromState, const std::string& toState);
private:
	static int nextId;
	bool EvaluateConditions(const std::vector<TransitionCondition>& conditions,
		const TransitionContext& ctx);
};
