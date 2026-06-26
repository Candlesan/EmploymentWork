#include "System/UI/BehaviorTreeEditor/BehaviorTreeEditor.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/ActionDerived.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include <algorithm>

void BehaviorTreeEditor::Draw(BehaviorTreeGraph& graph, BehaviorTree*& tree, Enemy* enemy)
{
	DrawToolbar(graph, tree, enemy);

	ed::SetCurrentEditor(context);
	ed::Begin("BehaviorTreeEditor",ImVec2(0,0));
	{
		// 描画を始める前にノードの位置を伝える
		if (graph.justLoad > 0)
		{
			for (auto& node : graph.nodes)
			{
				ed::SetNodePosition(node.nodeId, node.position);
			}
		}

		// ノードを描画
		for (auto& node : graph.nodes)
		{
			bool isActive = (enemy->GetLastInferredNode() != nullptr && enemy->GetLastInferredNode()->GetName() == node.name);
			// 色設定
			if (node.name == "Root")
				ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(80, 50, 10, 220));
			else if (isActive)
			{
				// 現在実行中のノードなら、背景をオレンジ、枠線を金色に発光させる！
				ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 120, 0, 255));
				ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 215, 0, 255));
			}
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
			if (isActive) {
				ed::PopStyleColor(2); // 背景 + 枠線
			}

			if (!graph.justLoad)
			{
				node.position = ed::GetNodePosition(node.nodeId);
			}
		}

		// リンクを描画
		for (auto& link : graph.links)
		{
			bool isLinkActive = false;

			// 線の行き先（endPin）を持っているノードを探す
			for (auto& node : graph.nodes)
			{
				if (node.pinIn == link.endPin)
				{
					if (enemy->GetActiveNode() != nullptr && enemy->GetActiveNode()->GetName() == node.name)
					{
						isLinkActive = true;
					}
					break;
				}
			}

			if (isLinkActive)
			{
				// 存在しないStyleColorの設定を消し、引数だけで太さ 4.0f のオレンジ線にします！
				ed::Link(link.linkId, link.startPin, link.endPin, ImColor(255, 140, 0, 255), 4.0f);
			}
			else
			{
				// 通常の線（デフォルト）
				ed::Link(link.linkId, link.startPin, link.endPin);
			}
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

		// ノード削除
		if (ed::BeginDelete())
		{
			ed::NodeId deletedNode;

			while (ed::QueryDeletedNode(&deletedNode))
			{
				bool canDelete = true;

				for (auto& node : graph.nodes)
				{
					// Rootは消さない
					if (node.nodeId == deletedNode && node.name == "Root")
					{
						canDelete = false;
						break;
					}
				}

				if (canDelete)
				{
					if (ed::AcceptDeletedItem())
					{
						graph.RemoveNode(deletedNode);
					}
				}
				else
				{
					ed::RejectDeletedItem();
				}
			}
		}
		ed::EndDelete();

		// リンク削除
		if (ed::BeginDelete())
		{
			ed::LinkId deletedLink;
			if (ed::QueryDeletedLink(&deletedLink))
			{
				if (ed::AcceptDeletedItem())
					graph.RemoveLink(deletedLink);
			}
		}
		ed::EndDelete();
	}

	// クリックされたノードを選択する
	ed::NodeId hoveredNode = ed::GetHoveredNode();

	// シングルクリックはノード選択
	if (ImGui::IsMouseClicked(0) && hoveredNode != ed::NodeId::Invalid)
	{		for (int i = 0; i < (int)graph.nodes.size(); i++)
		{
			if (graph.nodes[i].nodeId == hoveredNode)
			{
				selectedNodeIndex = i;
				break;
			}
		}
	}

	// ダブルクリックでシーケンサーを開く
	ed::NodeId doubleClickedNode = ed::GetDoubleClickedNode();
	if (doubleClickedNode != ed::NodeId::Invalid)
	{
		for (auto& node : graph.nodes)
		{
			if (node.nodeId == doubleClickedNode)
			{
				if (node.selectRule == BehaviorTree::SelectRule::Non)
				{
					requestOpenSequencer = true;
					selectedAnimName = node.name;
				}
			}
		}
	}

	ed::End();

	if (graph.justLoad > 0)
	{
		// カメラを自動調整する処理
		ed::NavigateToContent(0.0f);

		graph.justLoad--; // 用が済んだらフラグを戻す
	}

	ed::SetCurrentEditor(nullptr);

	DrawInspector(graph);
}

void BehaviorTreeEditor::ApplyToTree(BehaviorTree* tree, Enemy* enemy, const BehaviorTreeGraph& graph)
{
	// まず Root ノードだけ最初に追加する
	for (auto& node : graph.nodes)
	{
		if (node.name == "Root")
		{
			tree->AddNode(node.parentName, node.name, node.priority, node.selectRule, nullptr, nullptr);
			break;
		}
	}

	// 残りの全ノードを処理する（条件とアクションを同時に作る）
	for (auto& node : graph.nodes)
	{
		if (node.name == "Root") continue;

		// 条件（Judgment）の構築
		JudgmentBase* finalJudgment = nullptr;

		if (!node.conditions.empty())
		{
			AndJudgment* andJ = new AndJudgment(enemy);
			for (auto& cond : node.conditions)
			{
				if (cond.conditionType == (int)BTConditionType::None) {
					andJ->AddJudgment(new NoneJudgment(enemy));
				}
				else {
					andJ->AddJudgment(new FloatConditionJudgment(enemy, cond.conditionType, cond.judgmentValue, cond.compareType));
				}
			}
			finalJudgment = andJ;
		}
		else
		{
			finalJudgment = new NoneJudgment(enemy);
		}

		// アクション（Action）の構築
		ActionBase* finalAction = nullptr;

		// SelectRule が Non（末端ノード）の場合のみ、アクションを割り当てる
		if (node.selectRule == BehaviorTree::SelectRule::Non)
		{
			if (node.name == "Attack" || node.name == "Swing") { finalAction = new AttackAction(enemy, node.name); }
			else if (node.name == "Pursuit") { finalAction = new PursuitAction(enemy, node.name); }
			else if (node.name == "Wander") { finalAction = new WanderAction(enemy); }
			else if (node.name == "Idle") { finalAction = new IdleAction(enemy, node.name); }
			else if (node.name == "BloodSpear") { finalAction = new BloodSpearAction(enemy, node.name); }
		}

		// ツリーに登録（条件とアクションをセットで渡す！）
		tree->AddNode(node.parentName, node.name, node.priority, node.selectRule, finalJudgment, finalAction);
	}
}

void BehaviorTreeEditor::DrawToolbar(BehaviorTreeGraph& graph, BehaviorTree*& tree, Enemy* enemy)
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
		// 重複防止のためにグラフをクリア
		graph.nodes.clear();
		graph.links.clear();

		graph.Load("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");

		// 参照渡しの tree を安全にリセットする
		if (tree != nullptr)
		{
			delete tree;
			tree = nullptr; // 一度完全に nullptr にして安全を確保する
		}

		// まっさらな状態から再生成
		tree = new BehaviorTree();

		if (enemy != nullptr)
		{
			enemy->ClearActiveNode(); // 敵の実行状態の記憶を消す
		}

		// まっさらになったツリーに最新データを適用する
		ApplyToTree(tree, enemy, graph);

		// アニメーションデータも最新の数値で読み直す
		enemy->LoadAnimationData("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
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

		if (!animationNames.empty())
		{
			std::vector<const char*> comboItems;
			for (const auto& name : animationNames)
			{
				comboItems.push_back(name.c_str());
			}

			// 現在ノードに設定されているアニメーションが、リストの何番目にあるかを探す
			int currentSelection = 0;
			for (int i = 0; i < (int)animationNames.size(); i++)
			{
				if (animationNames[i] == node.config.animationName)
				{
					currentSelection = i;
					break;
				}
			}

			// 安全な Combo ボックスを表示して、選択されたら config.animationName を書き換える！
			if (ImGui::Combo(u8"再生するアニメーション", &currentSelection, comboItems.data(), (int)comboItems.size()))
			{
				node.config.animationName = animationNames[currentSelection];
			}

			// ループ設定
			ImGui::Checkbox(u8"ループ再生", &node.config.loop);

			// ルートモーション設定
			ImGui::Checkbox(u8"ルートモーションを使用", &node.config.useRootMotion);
			ImGui::Checkbox(u8"ルートモーション(拡張)", &node.config.useRootMotionEx);

			// ブレンド時間
			ImGui::DragFloat(u8"ブレンド時間(秒)", &node.config.blendTime, 0.01f, 0.0f, 2.0f);

			if (node.selectRule == BehaviorTree::SelectRule::Non)
			{
				ImGui::Separator();
				if (ImGui::Button(u8"シーケンサーを開く", ImVec2(-1, 0)))
				{
					requestOpenSequencer = true;
					selectedAnimName = node.name;
				}
				ImGui::Separator();
			}
		}
		else
		{
			// もしモデルのアニメーションリストが空っぽだった時のための安全対策
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), u8"※アニメーションリストが読み込まれていません。");
		}

		// AnimStateの編集
		{
			ImGui::Separator();
			ImGui::Text(u8"--- 中間ノードの遷移条件 (Judgment) ---");

			for (size_t i = 0; i < node.conditions.size(); i++)
			{
				ImGui::PushID(static_cast<int>(i));

				auto& cond = node.conditions[i];

				// 条件の名前のリスト
				const char* conditionNames[] = { u8"プレイヤーとの距離", u8"ボスのHPの割合(%)", u8"タイマー(秒)", u8"攻撃のクールダウン", u8"条件なし"};
				ImGui::Combo(u8"条件の種類", &cond.conditionType, conditionNames, IM_ARRAYSIZE(conditionNames));

				if (cond.conditionType != (int)BTConditionType::None)
				{
					const char* compareNames[] = { ">", "<", ">=", "<=", "==", "!=" };
					ImGui::Combo(u8"比較演算子", &cond.compareType, compareNames, IM_ARRAYSIZE(compareNames));

					ImGui::DragFloat(u8"判定する基準値", &cond.judgmentValue, 0.1f);
				}
				else
				{
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), u8"条件なし");
				}

				if (ImGui::Button(u8"条件を削除"))
				{
					node.conditions.erase(node.conditions.begin() + i);
					ImGui::PopID();
					break;
				}

				ImGui::PopID();
				ImGui::Separator();
			}

			if (ImGui::Button(u8"+ 条件を追加"))
			{
				BTCondition newCond;
				newCond.conditionType = (int)BTConditionType::None; // デフォで条件なしにしておく
				node.conditions.push_back(newCond);
			}
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
