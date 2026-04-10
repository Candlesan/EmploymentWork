#include "AnimationTransitionGraph.h"
#include <algorithm>

int AnimationTransitionGraph::nextId = 1;

// ノードの追加
void AnimationTransitionGraph::AddNode(int animState, ImVec2 pos)
{
	AnimNode node;
	node.nodeId = NextId();
	node.pinIn = NextId();
	node.pinOut = NextId();
	node.animState = animState;
	node.position = pos;
	nodes.push_back(node);
}

// 矢印の追加
void AnimationTransitionGraph::AddLink(ed::PinId from, ed::PinId to)
{
	// fromがどのノードか、toがどのノードかを探す
	int fromState = -1, toState = -1;
	for (auto& n : nodes)
	{
		if (n.pinOut == from) fromState = n.animState;
		if (n.pinIn == to) toState = n.animState;
	}
	if (fromState == -1 || toState == -1) return; // 見つからなければ何もしない

	AnimLink link;
	link.linkId = NextId();
	link.startPin = from;
	link.endPin = to;
	link.transition.fromState = fromState;
	link.transition.toState = toState;
	links.push_back(link);
}

// ノードの削除
void AnimationTransitionGraph::RemoveNode(ed::NodeId id)
{
	// ノードを消す前に繋がっているリンクを先に消す
	for (auto& node : nodes)
	{
		if (node.nodeId == id)
		{
			// このノードのpinInとpinOutを使ってるリンクを全部消す
			links.erase(std::remove_if(links.begin(), links.end(),
				[&](const AnimLink& l) {return l.startPin == node.pinOut || l.endPin == node.pinIn; }),
				links.end());
			break;
		}
	}

	// 削除対象を探す
	auto it = std::find_if(nodes.begin(), nodes.end(),
		[&](const AnimNode& n) {return n.nodeId == id; });

	if (it != nodes.end())
		nodes.erase(it);
}

// 矢印を消す
void AnimationTransitionGraph::RemoveLink(ed::LinkId id)
{
	// find_if:イテレーターの範囲内から指定された条件を満たす最初の要素を検索する関数
	auto it = std::find_if(links.begin(), links.end(),
		[&](const AnimLink& l) { return l.linkId == id; });
	if (it != links.end())
		links.erase(it);
}

// 条件を評価する関数
int AnimationTransitionGraph::EvaluateTransitions(int currentState, const TransitionContext& conditions)
{
	// currentStateから出るリンクだけ集める
	std::vector<AnimLink*> candidates;
	for (auto& link : links)
	{
		if (link.transition.fromState == currentState)
			candidates.push_back(&link);
	}

	// priorityの高い順にソート
	std::sort(candidates.begin(), candidates.end(), [](AnimLink* a, AnimLink* b) {
		return a->transition.priority > b->transition.priority;
		});

	// 条件を評価して最初に全部満たしたリンクのtoStateを返す
	for (auto* link : candidates)
	{
		if (EvaluateConditions(link->transition.conditions, conditions))
			return link->transition.toState;
	}
	return currentState;
}

// 一つずつ条件を確かめる関数
bool AnimationTransitionGraph::EvaluateConditions(
	const std::vector<TransitionCondition>& conditions,
	const TransitionContext& ctx)
{
	for (const auto& cond : conditions)
	{
		bool result = false;

		switch (cond.type)
		{
		case TransitionConditionType::AnimationFinished:
			result = (ctx.animSeconds >= ctx.animLength);
			break;

		case TransitionConditionType::AnimationTimeOver:
			result = (ctx.animSeconds >= cond.threshold);
			break;

		case TransitionConditionType::AnimationTimeIn:
			result = (ctx.animSeconds <= cond.threshold);
			break;
			
		case TransitionConditionType::MoveLengthOver:
			result = (ctx.moveLength > cond.threshold);
			break;

		case TransitionConditionType::MoveLengthUnder:
			result = (ctx.moveLength < cond.threshold);
			break;

		case TransitionConditionType::ButtonPressed:
			result = (ctx.buttonDown & cond.buttonMask) != 0;
			break;

		case TransitionConditionType::ButtonReleased :
			result = (ctx.buttonUp & cond.buttonMask) != 0;
			break;

		case TransitionConditionType::ButtonHeld:
			result = (ctx.buttonHeld & cond.buttonMask) != 0;
			break;

		case TransitionConditionType::BHold:
			result = ctx.bHold;
			break;

		case TransitionConditionType::BTap:
			result = ctx.bTap;
			break;

		case TransitionConditionType::RBTap:
			result = ctx.rbTap;
			break;

		case TransitionConditionType::RTHold:
			result = ctx.rtHold;
			break;

		case TransitionConditionType::RTTap:
			result = ctx.rtTap;
			break;

		case TransitionConditionType::JumpPressed:
			result = ctx.jumpPressed;
			break;

		case TransitionConditionType::StaminaEmpty:
			result = ctx.isStaminaEmpty;
			break;

		case TransitionConditionType::HasStamina:
			result = ctx.haveStamina;
			break;

		case TransitionConditionType::IsLockOn:
			result = ctx.isLockOn;
			break;

		case TransitionConditionType::CanRun:
			result = ctx.canRunTrigger;
			break;

		case TransitionConditionType::HavePotion:
			result = ctx.havePotion;
			break;

		case TransitionConditionType::HealCooldownReady:
			result = ctx.healCooldownReady;
			break;

		case TransitionConditionType::IsGuarding:
			result = ctx.isGuarding;
			break;	

		case TransitionConditionType::PositionX:

			result = (ctx.position.x > cond.threshold);
			break;

		case TransitionConditionType::PositionY:

			result = ((ctx.position.y > cond.threshold) );
			break;

		case TransitionConditionType::PositionZ:

			result = (ctx.position.z > cond.threshold);
			break;

		case TransitionConditionType::Always:
			result = true;
			break;
		}

		// negateが有効なら結果を反転
		if (cond.negate) result = !result;

		// AND条件なので1つでも偽なら即失敗
		if (!result) return false;
	}

	// 全条件を満たした
	return true;
}

// ノードエディタの保存
void AnimationTransitionGraph::Save(const std::string& path)
{
	using json = nlohmann::json;

	json j;
	j["nodes"] = json::array();
	for (auto& node : nodes)
	{
		j["nodes"].push_back({
			{"animState", node.animState},
			});
	}

	json linkJson;
	for (auto& link : links)
	{
		linkJson["fromState"] = link.transition.fromState;
		linkJson["toState"] = link.transition.toState;
		linkJson["priority"] = link.transition.priority;
		linkJson["colorR"] = link.color.x;
		linkJson["colorG"] = link.color.y;
		linkJson["colorB"] = link.color.z;
		linkJson["colorA"] = link.color.w;

		linkJson["conditions"] = json::array();
		for (auto& cond : link.transition.conditions)
		{
			linkJson["conditions"].push_back({
				{"type", (int)cond.type},
				{"threshold", cond.threshold},
				{"buttonMask", cond.buttonMask},
				{"negate", cond.negate},
				});
		}

		linkJson["Actions"] = json::array();
		for (auto& act : link.transition.actions)
		{
			linkJson["Actions"].push_back({
				{"type", (int)act.type},
				{"value", act.value},
				});
		}
		j["links"].push_back(linkJson);
	}

	std::ofstream file(path);
	file << j.dump(4);
}

// ノードエディタの読み込み
void AnimationTransitionGraph::Load(const std::string& path)
{
	using json = nlohmann::json;

	std::ifstream file(path);
	json j = json::parse(file);

	nodes.clear();
	links.clear();

	for (auto& node : j["nodes"])
	{
		AddNode(node["animState"], { 0, 0 });
	}

	for (auto& link : j["links"])
	{
		// nodesの中から対応するPinIdを探す
		int fromState = link["fromState"];
		int toState = link["toState"];

		ed::PinId fromPin, toPin;
		for (auto& node : nodes)
		{
			if (node.animState == fromState)
				fromPin = node.pinOut;
			if (node.animState == toState)
				toPin = node.pinIn;
		}

		// 見つかったPinIdでAddLinkを呼ぶ
		AddLink(fromPin, toPin);

		// AddLinkで追加された最後のlinkに条件とアクションを入れる
		AnimLink& newLink = links.back();

		newLink.transition.priority = link["priority"];
		newLink.color.x = link["colorR"];
		newLink.color.y = link["colorG"];
		newLink.color.z = link["colorB"];
		newLink.color.w = link["colorA"];

		for (auto& cond : link["conditions"])
		{
			TransitionCondition c;
			c.type = (TransitionConditionType)(int)cond["type"];
			c.threshold = cond["threshold"];
			c.buttonMask = cond["buttonMask"];
			c.negate = cond["negate"];
			newLink.transition.conditions.push_back(c);
		}

		for (auto& act : link["Actions"])
		{
			TransitionAction a;
			a.type = (TransitionActionType)(int)act["type"];
			a.value = act["value"];
			newLink.transition.actions.push_back(a);
		}
	}
}

// 自動整列
void AnimationTransitionGraph::AutoLayout()
{
	// 1行に並べるノード数
	const int columns = 4;
	const float nodeWidth = 200.0f; // ノード同士の横間隔
	const float nodeHeight = 120.0f; // ノード同士の縦間隔

	for (int i = 0; i < (int)nodes.size(); i++)
	{
		int col = i % columns;          // 何列目か
		int row = i / columns;          // 何行目か

		nodes[i].position = {
			col * nodeWidth + 100.0f,
			row * nodeHeight + 100.0f
		};

		// Node Editorに位置を反映する
		ed::SetNodePosition(nodes[i].nodeId, nodes[i].position);
	}
}

// 新しいグラフのセットアップ
void AnimationTransitionGraph::InitializeAsNew(const std::string& name)
{
	// 既存のデータをクリアする
	nodes.clear();
	links.clear();

	// 名前をセットする
	graphName = name;
}

// リンクの取得
const AnimationTransition* AnimationTransitionGraph::GetTransition(int fromState, int toState)
{
	for (auto& link : links)
	{
		if (link.transition.fromState == fromState &&
			link.transition.toState == toState)
		{
			return &link.transition;
		}
	}
	return nullptr;
}