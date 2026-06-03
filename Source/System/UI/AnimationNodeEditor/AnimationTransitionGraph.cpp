#include "AnimationTransitionGraph.h"
#include <algorithm>

int AnimationTransitionGraph::nextId = 1;

// ノードの追加
void AnimationTransitionGraph::AddNode(const std::string& StateName, ImVec2 pos)
{
	AnimNode node;
	node.nodeId = NextId();
	node.pinIn = NextId();
	node.pinOut = NextId();
	node.StateName = StateName;
	node.position = pos;
	nodes.push_back(node);
}

// 矢印の追加
void AnimationTransitionGraph::AddLink(ed::PinId from, ed::PinId to)
{
	// fromがどのノードか、toがどのノードかを探す
	std::string fromState = "", toState = "-1";
	for (auto& n : nodes)
	{
		if (n.pinOut == from) fromState = n.StateName;
		if (n.pinIn == to) toState = n.StateName;
	}
	if (fromState.empty()|| toState.empty()) return; // 見つからなければ何もしない

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
std::string AnimationTransitionGraph::EvaluateTransitions(const std::string& currentState, const TransitionContext& conditions)
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
		// ★変更: animState ではなく StateName を保存する
		j["nodes"].push_back({
			{"StateName", node.StateName},
			// ノードが持っている config も一緒に保存する！
			{"config", {
				{"animationName", node.config.animationName},
				{"loop", node.config.loop},
				{"useRootMotion", node.config.useRootMotion},
				{"useRootMotionEx", node.config.useRootMotionEx},
				{"blendTime", node.config.blendTime}
			}}
			});
	}

	json linkJson;
	for (auto& link : links)
	{
		// ★変更: リンクのつながりも fromState(文字列) で保存する
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
	if (!file.is_open()) return;

	json j;
	try {
		j = json::parse(file);
	}
	catch (const nlohmann::json::exception& e) {
		return; // JSONが壊れていたら何もしない（クラッシュ防止）
	}

	nodes.clear();
	links.clear();

	// -------- ノードの読み込み --------
	if (j.count("nodes") > 0) {
		for (auto& node : j["nodes"])
		{
			std::string stateName = "";

			// ★安全装置：新しい形式（StateName）か古い形式（animState）か自動判別する
			if (node.count("StateName") > 0 && node["StateName"].is_string()) {
				stateName = node["StateName"];
			}
			else if (node.count("animState") > 0 && node["animState"].is_number()) {
				stateName = std::to_string((int)node["animState"]);
			}
			else {
				stateName = "Unknown";
			}

			// ノードを追加
			AddNode(stateName, { 0, 0 });
			AnimNode& newNode = nodes.back();

			// configデータが存在すれば読み込む
			if (node.count("config") > 0) {
				auto& c = node["config"];
				newNode.config.animationName = c.value("animationName", "");
				newNode.config.loop = c.value("loop", false);
				newNode.config.useRootMotion = c.value("useRootMotion", false);
				newNode.config.useRootMotionEx = c.value("useRootMotionEx", false);
				newNode.config.blendTime = c.value("blendTime", 0.2f);
			}
		}
	}

	// -------- リンク（矢印）の読み込み --------
	if (j.count("links") > 0) {
		for (auto& link : j["links"])
		{
			std::string fromState = "";
			std::string toState = "";

			// ★安全装置：ここも新旧の形式に対応させる
			if (link.count("fromState") > 0) {
				if (link["fromState"].is_string()) fromState = link["fromState"];
				else if (link["fromState"].is_number()) fromState = std::to_string((int)link["fromState"]);
			}

			if (link.count("toState") > 0) {
				if (link["toState"].is_string()) toState = link["toState"];
				else if (link["toState"].is_number()) toState = std::to_string((int)link["toState"]);
			}

			ed::PinId fromPin, toPin;
			bool foundFrom = false, foundTo = false;

			// 文字列（StateName）でつながりを探す
			for (auto& node : nodes)
			{
				if (node.StateName == fromState) {
					fromPin = node.pinOut;
					foundFrom = true;
				}
				if (node.StateName == toState) {
					toPin = node.pinIn;
					foundTo = true;
				}
			}

			// 見つかった場合のみ AddLink を呼ぶ（クラッシュ対策）
			if (foundFrom && foundTo)
			{
				AddLink(fromPin, toPin);

				AnimLink& newLink = links.back();
				newLink.transition.priority = link.value("priority", 0);
				newLink.color.x = link.value("colorR", 1.0f);
				newLink.color.y = link.value("colorG", 1.0f);
				newLink.color.z = link.value("colorB", 1.0f);
				newLink.color.w = link.value("colorA", 1.0f);

				if (link.count("conditions") > 0) {
					for (auto& cond : link["conditions"])
					{
						TransitionCondition c;
						c.type = (TransitionConditionType)cond.value("type", 0);
						c.threshold = cond.value("threshold", 0.0f);
						c.buttonMask = cond.value("buttonMask", 0);
						c.negate = cond.value("negate", false);
						newLink.transition.conditions.push_back(c);
					}
				}

				if (link.count("Actions") > 0) {
					for (auto& act : link["Actions"])
					{
						TransitionAction a;
						a.type = (TransitionActionType)act.value("type", 0);
						a.value = act.value("value", 0.0f);
						newLink.transition.actions.push_back(a);
					}
				}
			}
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
const AnimationTransition* AnimationTransitionGraph::GetTransition(const std::string& fromState, const std::string& toState)
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