#include "System/UI/BehaviorTreeEditor/BehaviorTreeEditor.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/ActionDerived.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"


void BehaviorTreeEditor::Draw(BehaviorTreeGraph& graph)
{
	DrawToolbar(graph);

	ed::SetCurrentEditor(context);
	ed::Begin("BehaviorTreeEditor");
	{
		// ノードを描画
		for (auto& node : graph.nodes)
		{
			// 色設定
			if (node.name == "Root")
				ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(80, 50, 10, 220));
			else if (node.selectRule != BehaviorTree::SelectRule::Non)
				ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(0, 80, 160, 220));
			else
				ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(50, 50, 50, 220));

			ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
			ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);

			ed::BeginNode(node.nodeId);

			// INピン（上）
			if (node.name != "Root")
			{
				ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
				ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
				ed::BeginPin(node.pinIn, ed::PinKind::Input);
				ImGui::Dummy(ImVec2(60, 8));
				ed::EndPin();
				ed::PopStyleVar(2);
			}
			else
			{
				ImGui::Dummy(ImVec2(60, 8));
			}

			// ノード名
			ImGui::Dummy(ImVec2(0, 4));
			ImGui::TextUnformatted(node.name.c_str());
			ImGui::Dummy(ImVec2(0, 4));

			// OUTピン（下）
			if (node.name == "Root" || node.selectRule != BehaviorTree::SelectRule::Non)
			{
				ed::BeginPin(node.pinOut, ed::PinKind::Output);
				ImGui::Dummy(ImVec2(60, 8));
				ed::EndPin();
			}
			else
			{
				ImGui::Dummy(ImVec2(60, 8));
			}

			ed::EndNode();  

			ed::PopStyleVar(3);   
			ed::PopStyleColor(1); 
		}

		// リンクを描画
		for (auto& link : graph.links)
		{
			ed::Link(link.linkId, link.startPin, link.endPin);
		}

		// ドラックでリンクを作成
		if (ed::BeginCreate())
		{
			ed::PinId startPin, endPin;
			if (ed::QueryNewLink(&startPin, &endPin))
			{
				if (ed::AcceptNewItem())
					graph.AddLink(startPin, endPin);
			}
		}
		ed::EndCreate();
	}

	// クリックされたノードを選択する
	ed::NodeId clickedNode;
	if (ed::GetDoubleClickedNode() != ed::NodeId::Invalid) {}; // なんか今は無視らしい
	clickedNode = ed::GetHoveredNode();

	if (ImGui::IsMouseClicked(0) && clickedNode != ed::NodeId::Invalid)
	{
		for (int i = 0; i < (int)graph.nodes.size(); i++)
		{
			if (graph.nodes[i].nodeId == clickedNode)
			{
				selectedNodeIndex = i;
				break;
			}
		}
	}

	ed::End();
	ed::SetCurrentEditor(nullptr);

	DrawInspector(graph);
}

void BehaviorTreeEditor::ApplyToTree(BehaviorTree* tree, Enemy* enemy, const BehaviorTreeGraph& graph)
{
	for (auto& node : graph.nodes)
	{
		if (node.name == "Root")
		{
			tree->AddNode(node.parentName, node.name, node.priority, node.selectRule, nullptr, nullptr);
			break;
		}
	}

	for (auto& node : graph.nodes)
	{
		if (node.name == "Root") continue; // Rootは追加済みなのでスキップ

		JudgmentBase* judgment = nullptr;
		ActionBase* action = nullptr;

		// ノード名で対応するクラスを選ぶ
		if (node.name == "Attack") { judgment = new AttackJudgment(enemy);  action = new AttackAction(enemy); }
		else if (node.name == "Pursuit") { judgment = new PursuitJudgment(enemy); action = new PursuitAction(enemy, node.config.animationName); }
		else if (node.name == "Wander") { judgment = new WanderJudgment(enemy);  action = new WanderAction(enemy); }
		else if (node.name == "Idle")
		{
			action = new IdleAction(enemy, node.config.animationName); }

		tree->AddNode(node.parentName, node.name, node.priority,
			node.selectRule, judgment, action);
	}
}

void BehaviorTreeEditor::DrawToolbar(BehaviorTreeGraph& graph)
{
	if (ImGui::Button("Add Node"))
	{
		graph.AddNode("NewNode", { 100, 100 });
	}
	ImGui::SameLine();
	if (ImGui::Button("Save"))
	{
		graph.Save("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	}
	ImGui::SameLine();
	if (ImGui::Button("Load"))
	{
		graph.Load("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	}
}

void BehaviorTreeEditor::DrawInspector(BehaviorTreeGraph& graph)
{
	ImGui::Begin("BT Inspector");


	if (selectedNodeIndex >= 0 && selectedNodeIndex < (int)graph.nodes.size())
	{
		BTNode& node = graph.nodes[selectedNodeIndex];

		// ノード名を変える
		char nameBuf[128];
		strcpy_s(nameBuf, node.name.c_str()); // サイズを見ながら安全に文字列をコピーする
		if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
			node.name = nameBuf;

		// SelectRuleの編集
		const char* ruleNames[] = { "Non", "priority", "Sequence", "SequentialLooping", "Random" };
		int rule = (int)node.selectRule;
		if (ImGui::Combo("SelectRule", &rule, ruleNames, IM_ARRAYSIZE(ruleNames)))
			node.selectRule = (BehaviorTree::SelectRule)rule;

		ImGui::InputInt("Priority", &node.priority);

		// AnimStateの編集
		if (!animationNames.empty())
		{
			std::vector<const char*> items;
			for (auto& s : animationNames) items.push_back(s.c_str());

			int current = 0;
			for (int i = 0; i < (int)animationNames.size(); i++)
				if (animationNames[i] == node.animState) { current = i; break; }

			if (ImGui::Combo("AnimState", &current, items.data(), (int)items.size()))
				node.animState = animationNames[current];

			// アニメーション名 (GLTFの本当の名前)
			char nameBuf[256] = "";
			strncpy_s(nameBuf, node.config.animationName.c_str(), sizeof(nameBuf));
			if (ImGui::InputText(u8"アニメーション名 (GLTF)", nameBuf, sizeof(nameBuf)))
			{
				node.config.animationName = nameBuf;
			}

			// ループ設定
			ImGui::Checkbox(u8"ループ再生", &node.config.loop);

			// ルートモーション設定
			ImGui::Checkbox(u8"ルートモーションを使用", &node.config.useRootMotion);
			ImGui::Checkbox(u8"ルートモーション(拡張)", &node.config.useRootMotionEx);

			// ブレンド時間
			ImGui::DragFloat(u8"ブレンド時間(秒)", &node.config.blendTime, 0.01f, 0.0f, 2.0f);

		}
	}
	else
	{
		ImGui::Text("No node selected");
	}

	ImGui::End();
}

// ピンの描画ヘルパー関数
void BehaviorTreeEditor::DrawArrowPin(bool filled)
{

	ImVec2 p = ImGui::GetCursorScreenPos();
	float size = 12.0f;
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	// 下向き三角形（▼）
	ImVec2 p1 = ImVec2(p.x + size * 0.5f, p.y + size); // 下の頂点
	ImVec2 p2 = ImVec2(p.x, p.y);          // 左上
	ImVec2 p3 = ImVec2(p.x + size, p.y);          // 右上

	ImU32 color = filled
		? IM_COL32(255, 255, 255, 255)
		: IM_COL32(255, 255, 255, 100);

	if (filled)
		drawList->AddTriangleFilled(p1, p2, p3, color);
	else
		drawList->AddTriangle(p1, p2, p3, color, 1.5f);

	// ImGuiにこのサイズ分のスペースを確保させる
	ImGui::Dummy(ImVec2(size, size));
}
