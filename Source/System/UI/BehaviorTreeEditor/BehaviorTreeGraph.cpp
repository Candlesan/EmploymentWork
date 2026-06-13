#include "System/UI/BehaviorTreeEditor/BehaviorTreeGraph.h"

int BehaviorTreeGraph::nextId = 1;

// ノード追加
void BehaviorTreeGraph::AddNode(const std::string& StateName, ImVec2 pos)
{
	BTNode node;
	node.nodeId = NextId();
	node.pinIn = NextId();
	node.pinOut = NextId();
	node.animState = StateName;
	node.position = pos;
	nodes.push_back(node);
}

// 矢印の追加
void BehaviorTreeGraph::AddLink(ed::PinId from, ed::PinId to)
{
	// fromがどのノードか、toがどのノードかを探す
	std::string fromState = "", toState = "-1";
	for (auto& n : nodes)
	{
		if (n.pinOut == from) fromState = n.name;
		if (n.pinIn == to) toState = n.name;
	}
	if (fromState.empty() || toState.empty()) return; // 見つからなければ何もしない

	BTLink link;
	link.linkId = NextId();
	link.startPin = from;
	link.endPin = to;
	links.push_back(link);
}

// ノードの削除
void BehaviorTreeGraph::RemoveNode(ed::NodeId id)
{
	// ノードを消す前に繋がっているリンクを先に消す
	for (auto& node : nodes)
	{
		if (node.nodeId == id)
		{
			// このノードのpinInとpinOutを使ってるリンクを全部消す
			links.erase(std::remove_if(links.begin(), links.end(),
				[&](const BTLink& l) {return l.startPin == node.pinOut || l.endPin == node.pinIn; }),
				links.end());
			break;
		}
	}

	// 削除対象を探す
	auto it = std::find_if(nodes.begin(), nodes.end(),
		[&](const BTNode& n) {return n.nodeId == id; });

	if (it != nodes.end())
		nodes.erase(it);
}

// 矢印を消す
void BehaviorTreeGraph::RemoveLink(ed::LinkId id)
{
	// find_if:イテレーターの範囲内から指定された条件を満たす最初の要素を検索する関数
	auto it = std::find_if(links.begin(), links.end(),
		[&](const BTLink& l) { return l.linkId == id; });
	if (it != links.end())
		links.erase(it);
}

void BehaviorTreeGraph::Save(const std::string& path)
{

	// 全員のparentNameを一旦空っぽにリセットする
	for (auto& node : nodes)
	{
		if (node.name != "Root") node.parentName = "";
	}

	// リンクの情報を使って、正しい親の名前をセットしていく
	for (auto& link : links)
	{
		std::string parentName = "";

		// この線の出発点を持っているノードを探し、その名前を親の名前とする
		for (auto& n : nodes)
		{
			if (n.pinOut == link.startPin) parentName = n.name;
		}

		//　この線の終点を持っているノードを探し、親の名前を教えてあげる
		for (auto& n : nodes)
		{
			if (n.pinIn == link.endPin) n.parentName = parentName;
		}
	}

	using json = nlohmann::json;

	json j;
	j["nodes"] = json::array();
	for (auto& node : nodes)
	{
		j["nodes"].push_back({
			{"name", node.name},
			{"parentName", node.parentName},
			{"selectRule", (int)node.selectRule},
			{"priority", node.priority},
			{"StateName", node.animState},
			{"position", {node.position.x, node.position.y}},

				{"config", {
				   {"animationName", node.config.animationName},
				   {"loop", node.config.loop},
				   {"useRootMotion", node.config.useRootMotion},
				   {"useRootMotionEx", node.config.useRootMotionEx},
				   {"blendTime", node.config.blendTime}}
				}
						
		});
	}

	j["links"] = json::array();
	for (auto& link : links)
	{
		std::string fromName = "", toName = "";
		for (auto& n : nodes)
		{
			if (n.pinOut == link.startPin) fromName = n.name;
			if (n.pinIn == link.endPin)   toName = n.name;
		}
		j["links"].push_back({
			{"from", fromName},
			{"to",   toName}
		});

	}
		
	std::ofstream file(path);
	file << j.dump(4);
}

void BehaviorTreeGraph::Load(const std::string& path)
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

	if (j.count("nodes") > 0)
	{
		for (auto& nodeJson : j["nodes"])
		{
			float px = 0.0f, py = 0.0f;
			if (nodeJson.count("position") > 0 && nodeJson["position"].is_array())
			{
				px = nodeJson["position"][0].get<float>();
				py = nodeJson["position"][1].get<float>();
			}
			AddNode(nodeJson.value("name", "Unknown"), { px, py });
			
			BTNode& newNode = nodes.back();
			newNode.name = nodeJson.value("name", "");
			newNode.parentName = nodeJson.value("parentName", "");
			newNode.selectRule = (BehaviorTree::SelectRule)nodeJson.value("selectRule", 0);
			newNode.priority = nodeJson.value("priority", 1);
			newNode.animState = nodeJson.value("StateName", "");

			if (nodeJson.count("position") > 0 && nodeJson["position"].is_array() && nodeJson["position"].size() == 2)
			{
				newNode.position.x = nodeJson["position"][0].get<float>();
				newNode.position.y = nodeJson["position"][1].get<float>();
			}
			else
			{
				newNode.position.x = 0.0f;
				newNode.position.y = 0.0f;
			}

			// configデータが存在すれば読み込む
			if (nodeJson.count("config") > 0) {
				auto& c = nodeJson["config"];
				newNode.config.animationName = c.value("animationName", "");
				newNode.config.loop = c.value("loop", false);
				newNode.config.useRootMotion = c.value("useRootMotion", false);
				newNode.config.useRootMotionEx = c.value("useRootMotionEx", false);
				newNode.config.blendTime = c.value("blendTime", 0.2f);
			}

		}
	}

	if (j.count("links") > 0)
	{
		for (auto& link : j["links"])
		{
			std::string fromState = "";
			std::string toState = "";

			ed::PinId fromPin, toPin;
			bool foundFrom = false, foundTo = false;

			if (link.count("from") > 0) {
				if (link["from"].is_string()) fromState = link["from"];
				else if (link["from"].is_number()) fromState = std::to_string((int)link["from"]);
			}

			if (link.count("to") > 0) {
				if (link["to"].is_string()) toState = link["to"];
				else if (link["to"].is_number()) toState = std::to_string((int)link["to"]);
			}

			for (auto& n : nodes)
			{
				if (n.name == fromState) { fromPin = n.pinOut; foundFrom = true; }
				if (n.name == toState) { toPin = n.pinIn;  foundTo = true; }
			}

			if (foundFrom && foundTo)
				AddLink(fromPin, toPin);
		}
	}
}