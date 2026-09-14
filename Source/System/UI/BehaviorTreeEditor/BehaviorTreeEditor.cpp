#include "System/UI/BehaviorTreeEditor/BehaviorTreeEditor.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/ActionDerived.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/ActionFactory.h"

#include <set>
#include <algorithm>

void BehaviorTreeEditor::Draw(BehaviorTreeGraph& graph, BehaviorTree*& tree, Enemy* enemy)
{
	DrawToolbar(graph, tree, enemy);

	ed::SetCurrentEditor(context);
	ed::Begin(u8"ビヘイビアツリーエディター",ImVec2(0,0));
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

	if (isNeedRebuild)
	{
		if (tree != nullptr)
		{
			delete tree;
			tree = new BehaviorTree();
		}
		if (enemy != nullptr)
		{
			enemy->ClearActiveNode();
		}

		// エディター実行用ツリーに流し込む
		ApplyToTree(tree, enemy, graph);
		graph.UpdateParentNames();

		isNeedRebuild = false;
	}

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

	// 未登録のノードを集める
	std::vector<const BTNode*> pending;
	for (auto& node : graph.nodes)
	{
		if (node.name == "Root") continue;
		pending.push_back(&node);
	}

	// 登録済みノード名（親の存在を確認）
	std::set<std::string> addedNames;
	addedNames.insert("Root");

	// 親が先に登録されている必要があるので、追加出来なくなるまで繰り返す
	bool added = true;
	while (added && !pending.empty())
	{
		added = false;

		for (auto itr = pending.begin(); itr != pending.end();) // 更新の内容が変わるからループヘッダーには書かない
		{
			const BTNode& node = **itr; // BTNodeの実体を取り出す

			// 親がまだツリーになければ次の周回に回す
			if (addedNames.count(node.parentName) == 0)
			{
				++itr;
				continue;
			}

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
				finalAction = ActionFactory::Creat(node.actionType, enemy, node.name);
			}

			// ツリーに登録（条件とアクションをセットで渡す！）
			tree->AddNode(node.parentName, node.name, node.priority, node.selectRule, finalJudgment, finalAction, node.interruptible);

			addedNames.insert(node.name);
			itr = pending.erase(itr);
			added = true;
		}
	}

	// 残りの全ノードを処理する（条件とアクションを同時に作る）
	for (auto& node : graph.nodes)
	{
		if (node.name == "Root") continue;

		// 条件（Judgment）の構築
	}
}

void BehaviorTreeEditor::DrawToolbar(BehaviorTreeGraph& graph, BehaviorTree*& tree, Enemy* enemy)
{
	if (ImGui::Button(u8"ノードを追加"))
	{
		graph.AddNode("NewNode", { 100, 100 });
	}
	ImGui::SameLine();

	if (ImGui::Button(u8"保存"))
	{
		graph.Save("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	}
	ImGui::SameLine();

	if (ImGui::Button(u8"読込"))
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
		enemy->LoadAnimationData("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json", true);
	}
}

void BehaviorTreeEditor::DrawInspector(BehaviorTreeGraph& graph)
{
	ImGui::Begin(u8"ノード詳細");


	if (selectedNodeIndex >= 0 && selectedNodeIndex < (int)graph.nodes.size())
	{
		BTNode& node = graph.nodes[selectedNodeIndex];

		// ノード名を変える
		char nameBuf[128];
		strcpy_s(nameBuf, node.name.c_str()); // サイズを見ながら安全に文字列をコピーする
		if (ImGui::InputText(u8"ノード名", nameBuf, sizeof(nameBuf)))
			node.name = nameBuf;

		// ActionTypeの編集
		const char* actionTypes[] = {
			"None",
			"Idle",
			"MoveForward",
			"MoveBack",
			"MoveRight",
			"MoveLeft",
			"NormalMagicAttack",
			"RapidFire",
		};
		int currentActionType = (int)(node.actionType);
		if (ImGui::Combo(u8"アクションの種類", &currentActionType, actionTypes, IM_ARRAYSIZE(actionTypes)))
			node.actionType = (BTActionType)currentActionType;

		// SelectRuleの編集
		const char* ruleNames[] = {
			u8"末端##selectRule",
			u8"優先度順##selectRule",
			u8"順番に実行(シーケンス)##selectRule",
			u8"順繰り(ループシーケンス)##selectRule",
			u8"ランダム##selectRule"
		};

		int rule = (int)node.selectRule;
		if (ImGui::Combo(u8"実行ルール", &rule, ruleNames, IM_ARRAYSIZE(ruleNames)))
			node.selectRule = (BehaviorTree::SelectRule)rule;

		ImGui::InputInt(u8"優先順位", &node.priority);
		ImGui::Separator();

		ImGui::Checkbox(u8"途中中断可能", &node.interruptible);
		ImGui::Separator();

		if (!animationNames.empty() && node.selectRule == BehaviorTree::SelectRule::Non)
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

			ImGui::Separator();
			if (ImGui::Button(u8"シーケンサーを開く", ImVec2(-1, 0)))
			{
				requestOpenSequencer = true;
				selectedAnimName = node.name;
			}
			ImGui::Separator();
		}
		else if(node.selectRule == BehaviorTree::SelectRule::Non)
		{
			// もしモデルのアニメーションリストが空っぽだった時のための安全対策
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), u8"※アニメーションリストが読み込まれていません。");
		}

		// 遷移条件の編集
		{
			ImGui::Separator();
			ImGui::Text(u8"--- 中間ノードの遷移条件 (Judgment) ---");

			for (size_t i = 0; i < node.conditions.size(); i++)
			{
				ImGui::PushID(static_cast<int>(i));

				auto& cond = node.conditions[i];

				// 条件の名前のリスト
				const char* conditionNames[] = {
					u8"プレイヤーとの距離",
					u8"ボスのHPの割合(%)",
					u8"タイマー(秒)",
					u8"攻撃のクールダウン",
					u8"敵の現在の魔力残量",
					u8"敵の現在のヒート値",
					u8"敵の現在の防御魔法の耐久値",
					u8"プレイヤーの現在の魔力残量",
					u8"プレイヤーの現在のヒート値",
					u8"プレイヤーの現在の防御魔法の耐久値",
					u8"プレイヤーが防御魔法を展開中か (1.0=Yes, 0.0=No)",
					u8"自身に向かって飛んできている魔法の数",
					u8"条件なし"
				};
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
		ImGui::Text(u8"ノードが選択されていません");
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
