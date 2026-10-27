#include "AnimationTransitionEditor.h"

#include "GamePlay/Object/Character/Player/Player.h"

#include "System/Core/Input/Input.h"
#include "System/Graphic/Graphics.h"
#include "System/UI/Dialog/Dialog.h"

// ノードエディタ描画
std::string AnimationTransitionEditor::Draw(
	std::vector<AnimationTransitionGraph>& graphs,
	const std::string& activeState, const std::vector<int>& activeGraphStack,
	const std::string& upperState, const std::vector<int>& upperGraphStack)
{
	std::string doubleClickedState = "";

	// 現在の階層を表示
	ImGui::Text(u8"現在地:");
	for (int i = 0; i <= (int)graphStack.size(); i++)
	{
		ImGui::SameLine();
		// スタックの履歴か、現在のグラフかを取得
		int gIdx = (i < (int)graphStack.size()) ? graphStack[i] : currentGraphIndex;

		if (i == (int)graphStack.size()) ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", graphs[gIdx].graphName.c_str());
		else ImGui::Text("%s", graphs[gIdx].graphName.c_str());

		if (i < (int)graphStack.size())
		{
			ImGui::SameLine(); ImGui::Text(" > ");
		}
	}

	// 階層を戻るボタン
	if (!graphStack.empty())
	{
		if (ImGui::Button(u8"<- 親グラフに戻る"))
		{
			currentGraphIndex = graphStack.back(); // 親のインデックスに戻す
			graphStack.pop_back(); // 履歴を1つ消す
		}
		ImGui::Separator();
	}

	// Jsonに保存する
	if (ImGui::Button("Save"))
	{
		if (graphs.empty()) return "";

		// 現在選択中のグラフを参照して取得する
		auto& editGraph = graphs[currentGraphIndex];

		ed::SetCurrentEditor(context);

		for (auto& node : editGraph.nodes)
		{
			// エディターが側が持っている最新の座標を取得してデータの保存
			node.position = ed::GetNodePosition(node.nodeId);
		}

		ed::SetCurrentEditor(nullptr); // 使い終わったらもとにもどす

		char filepath[MAX_PATH] = { 0 };

		HWND hwnd = Graphics::Instance().GetWindowHandle();

		if (Dialog::SaveFileName(filepath, MAX_PATH,
			"JSON File\0*.json\0\0",	// フィルター
			"名前を付けて保存",	// タイトル
			"json",// 拡張子
			hwnd)				
			== DialogResult::OK)
		{
			editGraph.Save(filepath); // 編集中のやつを保存
		}
	}

	ImGui::SameLine();

	// Jsonを読み込む
	if (ImGui::Button("Load"))
	{
		char filepath[MAX_PATH] = { 0 };

		HWND hwnd = Graphics::Instance().GetWindowHandle();

		if (Dialog::OpenFileName(filepath, MAX_PATH,
			"JSON File\0*.json\0\0",	// フィルター
			"ファイルを開く",
			hwnd)	// タイトル
			== DialogResult::OK)
		{
			AnimationTransitionGraph newGraph;
			newGraph.Load(filepath);

			// ファイル名だけ取り出してgraphNameにセットする
			char filename[MAX_PATH];
			_splitpath_s(filepath, nullptr, 0, nullptr, 0, filename, MAX_PATH, nullptr, 0);
			newGraph.graphName = filename;

			graphs.push_back(newGraph); // リストに追加
			currentGraphIndex = (int)graphs.size() - 1; // 追加したやつを表示する
		}
	}

	ImGui::Separator();

	// タブ描画部分をこう変える
	int tabSelectedIndex = currentGraphIndex; // 別変数で受け取る

	// ファイルをタブで表示する
	if (ImGui::BeginTabBar("GraphTabs"))
	{

		for (int i = 0; i < (int)graphs.size(); i++)
		{
			if (ImGui::BeginTabItem(graphs[i].graphName.c_str()))
			{
				tabSelectedIndex = i; // ← currentGraphIndex は直接触らない
				ImGui::EndTabItem();
			}
		}
	}
	ImGui::EndTabBar();

	// タブの結果を反映するのは「graphStackが空のとき（トップ階層）のみ」にする
	if (graphStack.empty())
	{
		currentGraphIndex = tabSelectedIndex;
	}

	// ノードの追加
	if (ImGui::Button("+ Add Node"))
	{
		showSelectNodeWindow = !showSelectNodeWindow; // 追加ノード選択画面を開く
	}

	ImGui::SameLine();

	// ノードの追加
	if (ImGui::Button("+ Add SubGraph Node"))
	{
		ImGui::OpenPopup("Add SubGraph");
	}

	ImGui::SameLine();

	// グラフの追加
	if (ImGui::Button("+ Add Graph"))
	{
		ImGui::OpenPopup("Add Graph");
	}

	// Entryノードを出すボタン
	if (ImGui::Button("+ Add Entry"))
	{
		graphs[currentGraphIndex].AddNode("Entry", { 0, 0 });
	}

	ImGui::SameLine();

	// Exitノードを出すボタン
	if (ImGui::Button("+ Add Exit"))
	{
		graphs[currentGraphIndex].AddNode("Exit", { 0, 0 });
	}

	ImGui::SameLine();

	// ミックスノードを出すボタン
	if (ImGui::Button("+ Add Mix Node"))
	{
		graphs[currentGraphIndex].AddNode("MixNode", { 0, 0 });
		graphs[currentGraphIndex].nodes.back().type = NodeType::Mix;
	}

	// 自動整列
	if (ImGui::Button("Auto Layout"))
	{
		needAutoLayout = true; // フラグを立てるだけ
	}

	// 追加ノード選択画面
	if (showSelectNodeWindow)
	{
		ImGui::Begin("SelectNode");

		//Modelクラスから取得するように変更する
		auto model = Player::Instance().GetModel();

		if (model)
		{
			// モデルにあるアニメーションを取得する
			for (const auto& anim : model->GetAnimations())
			{
				// 既に追加済みのアニメーションかをチェックする
				bool alreadyAnimation = false;
				for (auto& node : graphs[currentGraphIndex].nodes)
				{
					// アニメーション名が一致するかをチェック
					if (node.config.animationName == anim.name)
					{
						alreadyAnimation = true;
						break;
					}
				}
				if (alreadyAnimation) continue;

				if (ImGui::Selectable(anim.name.c_str()))
				{
					graphs[currentGraphIndex].AddNode(anim.name, { 100, 100 }); // 選んだらノード追加

					// 追加したノードにアニメーション名を設定する
					graphs[currentGraphIndex].nodes.back().config.animationName = anim.name;

					showSelectNodeWindow = false; // 選んだら閉じる
				}
			}
		}

		ImGui::End();
	}

	if (ImGui::BeginPopupModal("Add SubGraph", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char newSubGraphName[64] = "";

		ImGui::Text(u8"階層ノードの名前を入力してください");
		ImGui::InputText("##subgraph_name_input", newSubGraphName, IM_ARRAYSIZE(newSubGraphName));
		ImGui::Separator();

		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			if (strlen(newSubGraphName) > 0)
			{
				// 新しいノードを追加
				graphs[currentGraphIndex].AddNode(newSubGraphName, { 100, 100 });

				// 作ったノードを取得して、最初から「階層タイプ」にする
				AnimNode& newNode = graphs[currentGraphIndex].nodes.back();
				newNode.type = NodeType::SubGraph;

				// パスを設定する
				newNode.subGraphPath = "Data/Json/Player/AnimationNodeEditor/" + std::string(newSubGraphName) + ".json";
			}

			newSubGraphName[0] = '\0';
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("CANCEL", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	AnimationTransitionGraph& currentGraph = graphs[currentGraphIndex];

	ed::SetCurrentEditor(context);
	ed::Begin("Animation Transition Editor");

	if (currentGraph.justLoad > 0)
	{
		for (auto& node : currentGraph.nodes)
		{
			ed::SetNodePosition(node.nodeId, node.position);
		}
	}

	// ノード描画
	for (auto& node : currentGraph.nodes)
	{
		// SubGraphノードなら背景を青っぽくする
		if (node.type == NodeType::SubGraph)
			ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));

		bool isActive = false;
		bool isUpperGraph = false;
		for (int upperIdx : upperGraphStack) {
			if (upperIdx == currentGraphIndex) { isUpperGraph = true; break; }
		}

		if (node.type == NodeType::SubGraph)
		{
			if (!node.subGraphPath.empty())
			{
				char filename[MAX_PATH];
				_splitpath_s(node.subGraphPath.c_str(), nullptr, 0, nullptr, 0,
					filename, MAX_PATH, nullptr, 0);
				std::string childGraphName = filename;

				const auto& targetStack = isUpperGraph ? upperGraphStack : activeGraphStack;
				for (int activeIdx : targetStack)
				{
					if (graphs[activeIdx].graphName == childGraphName)
					{
						isActive = true;
						break;
					}
				}
			}
		}
		else
		{
			std::string targetActiveState = isUpperGraph ? upperState : activeState;
			isActive = (node.StateName == targetActiveState);
		}

		if (isActive)
		{
			ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 4.0f);
		}

		// 線を直線にする設定
		ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
		ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
		ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f); // 0にすると曲線が直線になる

		ed::BeginNode(node.nodeId);

		ImGui::Text("State: %s", node.StateName.c_str());

		// 矢印の設定は「INピンを描画する直前」に書く！
		ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);  // 矢印の大きさ
		ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f); // 矢印の太さ

		if (node.type == NodeType::Mix)
		{
			ed::BeginPin(node.pinIn, ed::PinKind::Input);
			ImGui::Text("A");
			ed::EndPin();

			ImGui::Spacing();

			ed::BeginPin(node.pinInB, ed::PinKind::Input);
			ImGui::Text("B");
			ed::EndPin();

			ImGui::PushItemWidth(100.0f);
			ImGui::SliderFloat("##mixRate", &node.mixRate, 0.0f, 1.0f);
			ImGui::PopItemWidth();
		}
		else // 普通のノードならいつも通りにInを描画する
		{
			ed::BeginPin(node.pinIn, ed::PinKind::Input);
			ImGui::Text("IN");
			ed::EndPin();
		}


		// INピンの描画が終わったら、忘れずに矢印の設定を戻す（2個）
		ed::PopStyleVar(2);


		ImGui::SameLine();

		ed::BeginPin(node.pinOut, ed::PinKind::Output);
		ImGui::Text("OUT");
		ed::EndPin();


		ed::EndNode(); // ノードの描画終了

		// ノードの描画が終わったら、直線用の設定を戻す（3個）
		ed::PopStyleVar(3);


		// isActiveの色や太さを戻す
		if (isActive)
		{
			ed::PopStyleColor();
			ed::PopStyleVar();
		}

		// SubGraphノードの色をもとに戻す
		if (node.type == NodeType::SubGraph)
		{
			ed::PopStyleColor();
		}
	}

	// リンク描画
	for (auto& link : currentGraph.links)
	{
		// 今いるグラフのインデックスを取得
		// activeGraphStackの中にcurrentGraphIndexがあるか探す
		std::string evalState = activeState; // デフォルトは currentState

		// 親グラフを見てるとき → evalStateを子グラフ名に差し替える
		for (int i = 0; i < (int)activeGraphStack.size() - 1; i++)
		{
			if (activeGraphStack[i] == currentGraphIndex)
			{
				// 1個下の子グラフ名がこの階層でのevalState
				evalState = graphs[activeGraphStack[i + 1]].graphName;
				break;
			}
		}

		// このリンクが現在の状態からの遷移かを判定する
		bool isActiveLink = (link.transition.fromState == evalState);

		// 描画色を決める
		ImVec4 drawColor = link.color;
		if (isActiveLink)
		{
			drawColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f); // ノードのハイライト色と合わせる例
		}

		// リンクを描画（色を渡す）
		ed::Link(link.linkId, link.startPin, link.endPin, drawColor);
	}

	// ドラックでリンクを作成
	if (ed::BeginCreate())
	{
		ed::PinId startPin, endPin;
		if (ed::QueryNewLink(&startPin, &endPin))
		{
			if (ed::AcceptNewItem())
				currentGraph.AddLink(startPin, endPin);
		}
	}
	ed::EndCreate();

	// ノード削除
	if (ed::BeginDelete())
	{
		ed::NodeId deletedNode;
		if (ed::QueryDeletedNode(&deletedNode))
		{
			if (ed::AcceptDeletedItem())
				currentGraph.RemoveNode(deletedNode);
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
				currentGraph.RemoveLink(deletedLink);
		}
	}
	ed::EndDelete();

	// 右クリックメニュー
	ed::LinkId rightClickedLinkId;
	if (ed::ShowLinkContextMenu(&rightClickedLinkId))
	{
		// クリックされたリンクを探してメンバ変数に保存
		for (auto& link : currentGraph.links)
		{
			if (link.linkId == rightClickedLinkId)
			{
				copiedLink = &link;
				break;
			}
		}
		OpenCopyMenu = true;
	}

	ed::End();

	// 自動整列
	if (needAutoLayout)
	{
		currentGraph.AutoLayout();
		needAutoLayout = false;
	}

	// AddGraphのポップ
	if (ImGui::BeginPopupModal("Add Graph", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		static char newName[64] = ""; // 入力用の一時バッファ

		ImGui::Text(u8"グラフ名を入力してください");

		ImGui::InputText("##graph_name_input", newName, IM_ARRAYSIZE(newName));

		ImGui::Separator();

		// OKを押した時の処理
		if (ImGui::Button("OK", ImVec2(120, 0)))
		{
			// AddGraphを呼ぶ
			Player::Instance().AddGraph(newName);

			// グラフを追加したらバッファをクリアしポップを閉じる
			newName[0] = '\0';
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		// Cancelを押した時の処理
		if (ImGui::Button("CANCEL", ImVec2(120, 0)))
		{
			// 何もせずポップを閉じる
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (OpenCopyMenu)
	{
		ImGui::Begin("CopeMenu");
		ImGui::Text(u8"コピー先のグラフ");
		ImGui::Separator();

		for (int i = 0; i < (int)graphs.size(); i++)
		{
			if (i == currentGraphIndex) continue; // 自分自身はスキップする

			bool selected = ImGui::MenuItem(graphs[i].graphName.c_str());

			if (selected)
			{
				// コピー処理
				if (copiedLink)
				{
					// コピー先グラフ
					auto& targetGraph = graphs[i];

					// コピー先にfromStateとtoStateのノードを探す
					ed::PinId fromPin, toPin;
					bool foundFrom = false, foundTo = false;

					for (auto& node : targetGraph.nodes)
					{
						if (node.StateName == copiedLink->transition.fromState)
						{
							fromPin = node.pinOut;
							foundFrom = true;
						}

						if (node.StateName == copiedLink->transition.toState)
						{
							toPin = node.pinIn;
							foundTo = true;
						}
					}

					// 両方見つかったらリンクを追加
					if (foundFrom && foundTo)
					{
						// リンク追加
						targetGraph.AddLink(fromPin, toPin);

						// 条件とアクションをコピー
						// 追加したリンクを取得する
						AnimLink& newLink = targetGraph.links.back();

						// 条件をコピーする
						newLink.transition.conditions = copiedLink->transition.conditions;

						// アクションをコピーする
						newLink.transition.actions = copiedLink->transition.actions;

						// 色をコピーする
						newLink.color = copiedLink->color;

						OpenCopyMenu = false;
					}
				}
			}
		}

		if (ImGui::Button(u8"閉じる")) OpenCopyMenu = false;

		ImGui::End();
	}

	// ノードをダブルクリックするとシーケンサーを起動
	ed::NodeId doubleClickedNode = ed::GetDoubleClickedNode();

	if (doubleClickedNode)
	{
		// ダブルクリックされたノードを探す
		for (auto& node : currentGraph.nodes)
		{
			if (node.nodeId == doubleClickedNode)
			{
				if (node.type == NodeType::SubGraph)
				{
					if (node.subGraphPath.empty()) break;

					// 子グラフを作って読み込む
					char filename[MAX_PATH];
					_splitpath_s(node.subGraphPath.c_str(), nullptr, 0, nullptr, 0, filename, MAX_PATH, nullptr, 0);
					std::string targetName = filename;

					// 既にロードされているグラフの中から探す
					int targetIdx = -1;
					for (int i = 0; i < (int)graphs.size(); i++)
					{
						if (graphs[i].graphName == targetName)
						{
							targetIdx = i;
							break;
						}
					}

					if (targetIdx == -1)
					{
						AnimationTransitionGraph newGraph;
						newGraph.Load(node.subGraphPath); // 既存ファイルがあればロード、無ければ空
						newGraph.graphName = targetName;
						graphs.push_back(newGraph);
						targetIdx = (int)graphs.size() - 1; // 一番最後に追加されたインデックス
					}

					// 見つかったらそのグラフに移動する
					if (targetIdx != -1)
					{
						graphStack.push_back(currentGraphIndex); // 今のインデックスを履歴に積む
						currentGraphIndex = targetIdx;           // グラフを切り替える
						ed::NavigateToContent();                 // カメラを合わせる
					}
				}
				else
				{
					// シーケンサーの現在のステートをこのノードの名前に変える
					doubleClickedState = node.StateName; // 見つけたら名前を保存する
				}
				break;
			}
		}
	}

	if (currentGraph.justLoad > 0)
	{
		ed::NavigateToContent(0.0f);
		currentGraph.justLoad--;  // 毎フレーム減らす
	}

	DrawSelectedLinkEditor(currentGraph);
	DrawSelectedNodeEditor(currentGraph);

	ed::SetCurrentEditor(nullptr);

	return doubleClickedState;
}

// リンク選択
void AnimationTransitionEditor::DrawSelectedLinkEditor(AnimationTransitionGraph& graph)
{
	// 選択中のリンクを取得
	ed::LinkId selectedLinkId;
	if (ed::GetSelectedLinks(&selectedLinkId, 1) == 0) return; // 何も選択されていなければ何もしない

	// 選択されたリンクをgraph.linksから探す
	AnimLink* selectedLink = nullptr;
	for (auto& link : graph.links)
	{
		if (link.linkId == selectedLinkId)
		{
			selectedLink = &link;
			break;
		}
	}
	if (!selectedLink) return;

	AnimationTransition& trans = selectedLink->transition;

	// 条件編集ウィンドウ
	ImGui::Begin("Transition Condition Editor");
	{
		ImGui::Text("From State: %s -> To State: %s", trans.fromState.c_str(), trans.toState.c_str());
		ImGui::ColorEdit4("Link Color", &selectedLink->color.x);
		ImGui::DragInt("Priority", &trans.priority);
		ImGui::Separator();

		ImGui::Text(u8"Actions(遷移時に実行する処理)");
		for (int i = 0; i < (int)trans.actions.size(); i++)
		{
			auto& action = trans.actions[i];
			ImGui::PushID(i + 1000); // conditionsとIDを被らないようにするため

			const char* actionTypes[] = {
				"None",
				"MoveSpeed",
				"TurnSpeed",
				"Avoid",
			};
			int actionIndex = (int)action.type;
			if (ImGui::Combo("Action", &actionIndex, actionTypes, IM_ARRAYSIZE(actionTypes)))
				action.type = (TransitionActionType)actionIndex;

			switch (action.type)
			{
			case TransitionActionType::MoveSpeed:
			case TransitionActionType::TurnSpeed:
				ImGui::DragFloat("Amout", &action.value, 0.1f, 0.0f, 1000.0f);
				break;

			case TransitionActionType::SetIsAvoid:
			{
				bool avoidFlag = (action.value != 0.0f);
				if (ImGui::Checkbox("Avoid", &avoidFlag))
				{
					action.value = avoidFlag ? 1.0f : 0.0f;
				}
				break;
			}
			}

			if (ImGui::Button("Remove Action"))
			{
				trans.actions.erase(trans.actions.begin() + i);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
		}

		ImGui::Separator();

		// 条件リストを表示
		ImGui::Text(u8"Condition(アニメーションの遷移条件)");
		for (int i = 0; i < (int)trans.conditions.size(); i++)
		{
			TransitionCondition& cond = trans.conditions[i];
			ImGui::PushID(i);

			const ComboItem condItems[] = {
				{ "AnimationFinished",	TransitionConditionType::AnimationFinished },         
				{ "AnimationTimeOver",	TransitionConditionType::AnimationTimeOver },
				{ "AnimationTimeIn",	TransitionConditionType::AnimationTimeIn },

				{ "ButtonPressed",		TransitionConditionType::ButtonPressed },
				{ "ButtonReleased",		TransitionConditionType::ButtonReleased },
				{ "ButtonHeld",			TransitionConditionType::ButtonHeld },

				{ "MoveSpeed",			TransitionConditionType::MoveSpeed },   
				{ "MoveLengthOver",		TransitionConditionType::MoveLengthOver },
				{ "MoveLengthUnder",	TransitionConditionType::MoveLengthUnder },   
				{ "EnemyFromDistance",	TransitionConditionType::EnemyFromDistance },

				{ "IsGround",			TransitionConditionType::IsGround },
				{ "IsLockOn",			TransitionConditionType::IsLockOn },
				{ "IsGuarding",			TransitionConditionType::IsGuarding },
				{ "IsCancelable",		TransitionConditionType::IsCancelable },

				{ "InputDir",			TransitionConditionType::InputDir },          
				{ "PositionX",			TransitionConditionType::PositionX },    
				{ "PositionY",			TransitionConditionType::PositionY },           
				{ "PositionZ",			TransitionConditionType::PositionZ },

				{ "IsFalling",			TransitionConditionType::IsFalling },
				{ "IsDown",			TransitionConditionType::IsDown },
				{ "IsDead",			TransitionConditionType::IsDead },
				{ "currentHP",			TransitionConditionType::currentHP },

				{ "Always",				TransitionConditionType::Always },
			};

			int currentIndex = 0;
			for (int i = 0; i < IM_ARRAYSIZE(condItems); i++)
			{
				if (condItems[i].type == cond.type)
				{
					currentIndex = i;
					break;
				}
			}

			// ドロップダウンのラベル一覧を作成
			const char* comboLabels[IM_ARRAYSIZE(condItems)];
			for (int i = 0; i < IM_ARRAYSIZE(condItems); i++)
			{
				comboLabels[i] = condItems[i].label;
			}

			// ImGui描画
			if (ImGui::Combo("Type", &currentIndex, comboLabels, IM_ARRAYSIZE(condItems))) {
				// 選択された項目の Enum 値をセットする！
				cond.type = condItems[currentIndex].type;
			}

			// タイプによって追加入力欄を出す
			switch (cond.type)
			{
			case TransitionConditionType::AnimationTimeOver:
			case TransitionConditionType::AnimationTimeIn:
			case TransitionConditionType::MoveLengthOver:
			case TransitionConditionType::MoveLengthUnder:
			case TransitionConditionType::EnemyFromDistance:
			case TransitionConditionType::MoveSpeed:
			case TransitionConditionType::currentHP:
				ImGui::DragFloat("Threshold", &cond.threshold, 0.01f);
				break;

			case TransitionConditionType::ButtonPressed:
			case TransitionConditionType::ButtonReleased:
			{
				// ボタン選択Comboを表示
				const char* buttonNames[] = { "A","B","X","Y","LB","RB","LT","RT" };
				const int   buttonMasks[] = {
					GamePad::BTN_A, GamePad::BTN_B,
					GamePad::BTN_X, GamePad::BTN_Y,
					GamePad::BTN_LEFT_SHOULDER,  GamePad::BTN_RIGHT_SHOULDER,
					GamePad::BTN_LEFT_TRIGGER,   GamePad::BTN_RIGHT_TRIGGER,
				};

				if (cond.buttonMask == 0) {
					cond.buttonMask = buttonMasks[0];
				}

				int currentIndex = 0;
				for (int i = 0; i < IM_ARRAYSIZE(buttonMasks); i++)
				{
					if (cond.buttonMask == buttonMasks[i]) { currentIndex = i; break; }
				}
				if (ImGui::Combo("Button", &currentIndex, buttonNames, IM_ARRAYSIZE(buttonNames)))
					cond.buttonMask = buttonMasks[currentIndex];
				break;
			}
			case TransitionConditionType::ButtonHeld:
			{
				// ボタン選択Comboを表示
				const char* buttonNames[] = { "A","B","X","Y","LB","RB","LT","RT" };
				const int   buttonMasks[] = {
					GamePad::BTN_A, GamePad::BTN_B,
					GamePad::BTN_X, GamePad::BTN_Y,
					GamePad::BTN_LEFT_SHOULDER,  GamePad::BTN_RIGHT_SHOULDER,
					GamePad::BTN_LEFT_TRIGGER,   GamePad::BTN_RIGHT_TRIGGER,
				};

				if (cond.buttonMask == 0) {
					cond.buttonMask = buttonMasks[0];
				}

				int currentIndex = 0;
				for (int i = 0; i < IM_ARRAYSIZE(buttonMasks); i++)
				{
					if (cond.buttonMask == buttonMasks[i]) { currentIndex = i; break; }
				}
				if (ImGui::Combo("Button", &currentIndex, buttonNames, IM_ARRAYSIZE(buttonNames)))
					cond.buttonMask = buttonMasks[currentIndex];

				ImGui::Separator();

				ImGui::DragFloat(u8"長押し時間(秒)", &cond.threshold, 0.05f, 0.0f, 3.0f, "%.2f秒");
				break;
			}

			case TransitionConditionType::PositionX:
			case TransitionConditionType::PositionY:
			case TransitionConditionType::PositionZ:
				ImGui::DragFloat("Thershold", &cond.threshold, 0.01f, 0.0f, 1000.0f);
				break;

			case TransitionConditionType::InputDir:
			{
				const char* dirNames[] = {
					"None",
					"Forward",
					"ForwardRight",
					"ForwardLeft",
					"Right",
					"Backward",
					"BackwardRight",
					"BackwardLeft",
					"Left" };
				int currentDirIdx = (int)cond.targetDir;

				if (ImGui::Combo("Direction", &currentDirIdx, dirNames, IM_ARRAYSIZE(dirNames)));
				{
					cond.targetDir = (RelativeInputDir)currentDirIdx;
				}

				break;
			}
			}

			// 条件の反転チェックボックス
			ImGui::Checkbox(u8"Negate（条件を反転）", &cond.negate);

			// 条件を削除するボタン
			if (ImGui::Button("Remove"))
			{
				trans.conditions.erase(trans.conditions.begin() + i);
				ImGui::PopID();
				break; // イテレーターが壊れるからbreakにする
			}

			ImGui::Separator();
			ImGui::PopID();
		}

		// 条件を追加するボタン
		if (ImGui::Button("+ Add Condition"))
		{
			trans.conditions.push_back(TransitionCondition{});
		}

		// 遷移時の条件の追加
		if (ImGui::Button("+ Add Action"))
		{
			trans.actions.push_back(TransitionAction{});
		}
	}

	ImGui::End();
}

// ノード選択
void AnimationTransitionEditor::DrawSelectedNodeEditor(AnimationTransitionGraph& graph)
{
	// 選択中のノードを取得（1つだけ）
	ed::NodeId selectedNodeId;
	if (ed::GetSelectedNodes(&selectedNodeId, 1) == 0) return; // 何も選択されていなければ何もしない

	// 選択されたノードをgraph.nodesから探す
	AnimNode* selectedNode = nullptr;
	for (auto& node : graph.nodes)
	{
		if (node.nodeId == selectedNodeId)
		{
			selectedNode = &node;
			break;
		}
	}
	if (!selectedNode) return;

	// プロパティ編集ウィンドウを表示
	ImGui::Begin("Node Properties (Animation Config)");
	{
		ImGui::Text(u8"ステート名: %s", selectedNode->StateName.c_str());
		ImGui::Separator();

		char assetBuf[128];
		strncpy_s(assetBuf, selectedNode->assetName.c_str(), sizeof(assetBuf));

		ImGui::Text(u8"再生アセット名（GLTF）：");
		ImGui::SameLine();
		if (ImGui::InputText(u8"##再生アセット名入力欄", assetBuf, sizeof(assetBuf)))
		{
			selectedNode->assetName = assetBuf;
		}
		ImGui::Separator();

		// ノードの種類を切り替えるボタン
		if (ImGui::Button(selectedNode->type == NodeType::Animation ? u8"階層(SubGraph)に変換" : u8"通常(Animation)に変換"))
		{
			selectedNode->type = (selectedNode->type == NodeType::Animation) ? NodeType::SubGraph : NodeType::Animation;
		}

		ImGui::Separator();

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), u8"【このノードから出ている矢印一覧】");

		int linkIndex = 1;
		for (auto& link : graph.links)
		{
			// 選択中のノードの出力ピンから出ている矢印を探す
			if (link.startPin == selectedNode->pinOut)
			{
				ImGui::PushID((int)link.linkId.Get());

				// 遷移先のノード名を取得
				std::string toName = "Unknown";
				for (auto& n : graph.nodes) {
					if (n.pinIn == link.endPin) {
						toName = n.StateName;
						break;
					}
				}

				// ボタンのラベルを作成
				char btnLabel[128];
				snprintf(btnLabel, sizeof(btnLabel), u8"▶ To: %s (矢印 %d)", toName.c_str(), linkIndex);

				// 今選択されている矢印なら緑色っぽくして分かりやすくする
				bool isSelected = ed::IsLinkSelected(link.linkId);
				if (isSelected) {
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
				}

				// ボタンを表示
				if (ImGui::Button(btnLabel, ImVec2(-1, 0)))
				{
					ed::ClearSelection();
					ed::SelectLink(link.linkId);
				}

				if (isSelected) {
					ImGui::PopStyleColor();
				}

				ImGui::PopID();
				linkIndex++;
			}
		}

		// 種類によって表示するUIを変える
		if (selectedNode->type == NodeType::SubGraph)
		{
			ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), u8"【階層ノード】");
			ImGui::Text(u8"ダブルクリックでこのファイルを開きます。");

			char pathBuf[MAX_PATH] = "";
			strncpy_s(pathBuf, selectedNode->subGraphPath.c_str(), sizeof(pathBuf));
			if (ImGui::InputText(u8"子グラフのファイルパス", pathBuf, sizeof(pathBuf)))
			{
				selectedNode->subGraphPath = pathBuf;
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.5f, 1.0f), u8"【アニメーションノード】");

			// アニメーション名 (GLTFの本当の名前)
			char nameBuf[256] = "";
			strncpy_s(nameBuf, selectedNode->config.animationName.c_str(), sizeof(nameBuf));
			if (ImGui::InputText(u8"アニメーション名 (GLTF)", nameBuf, sizeof(nameBuf)))
			{
				selectedNode->config.animationName = nameBuf;
			}

			// ループ設定
			ImGui::Checkbox(u8"ループ再生", &selectedNode->config.loop);

			// ルートモーション設定
			ImGui::Checkbox(u8"ルートモーションを使用", &selectedNode->config.useRootMotion);
			ImGui::Checkbox(u8"ルートモーション(拡張)", &selectedNode->config.useRootMotionEx);
			ImGui::Checkbox(u8"Y移動を無視", &selectedNode->config.bakeTranslationY);

			// ブレンド時間
			ImGui::DragFloat(u8"ブレンド時間(秒)", &selectedNode->config.blendTime, 0.01f, 0.0f, 2.0f);

			// レイヤー分けする親ボーンを入力する欄
			char BoneNameBuf[256] = "";
			strncpy_s(BoneNameBuf, selectedNode->config.targetBoneName.c_str(), sizeof(BoneNameBuf));

			if(ImGui::InputText(u8"ボーン名入力(レイヤー分け)", BoneNameBuf, sizeof(BoneNameBuf)))
			{
				selectedNode->config.targetBoneName = BoneNameBuf;
			}
		}
	}
	ImGui::End();
}
