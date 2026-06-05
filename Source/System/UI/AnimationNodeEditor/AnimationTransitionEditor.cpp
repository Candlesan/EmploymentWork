#include "AnimationTransitionEditor.h"

#include "GamePlay/Object/Character/Player/Player.h"

#include "System/Core/Input/Input.h"
#include "System/Graphic/Graphics.h"
#include "System/UI/Dialog.h"

// ノードエディタ描画
std::string AnimationTransitionEditor::Draw(std::vector<AnimationTransitionGraph>& graphs, const std::string& activeState)
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

		char filepath[MAX_PATH] = { 0 };

		HWND hwnd = Graphics::Instance().GetWindowHandle();

		if (Dialog::SaveFileName(filepath, MAX_PATH,
			"JSON File\0*.json\0\0",	// フィルター
			u8"名前を付けて保存",	// タイトル
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
			u8"ファイルを開く",
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

	// 選択中のグラフを描画する
	AnimationTransitionGraph& currentGraph = graphs[currentGraphIndex];

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
		currentGraph.AddNode("Entry", { 50, 200 });
	}

	ImGui::SameLine();

	// Exitノードを出すボタン
	if (ImGui::Button("+ Add Exit"))
	{
		currentGraph.AddNode("Exit", { 400, 200 });
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
				for (auto& node : currentGraph.nodes)
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
					currentGraph.AddNode(anim.name, { 100, 100 }); // 選んだらノード追加

					// 追加したノードにアニメーション名を設定する
					currentGraph.nodes.back().config.animationName = anim.name;

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
				currentGraph.AddNode(newSubGraphName, { 100, 100 });

				// 作ったノードを取得して、最初から「階層タイプ」にする
				AnimNode& newNode = currentGraph.nodes.back();
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

	ed::SetCurrentEditor(context);
	ed::Begin("Animation Transition Editor");

	// ノード描画
	for (auto& node : currentGraph.nodes)
	{
		// SubGraphノードなら背景を青っぽくする
		if (node.type == NodeType::SubGraph)
			ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(0.2f, 0.4f, 0.6f, 1.0f));

		bool isActive = (node.StateName == activeState);
		if (isActive)
		{
			ed::PushStyleColor(ed::StyleColor_NodeBg, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_NodeBorderWidth, 4.0f);
		}

		ed::BeginNode(node.nodeId);

		ImGui::Text("State: %s", node.StateName.c_str());

		ed::BeginPin(node.pinIn, ed::PinKind::Input);
		ImGui::Text("IN");
		ed::EndPin();

		ImGui::SameLine();

		ed::BeginPin(node.pinOut, ed::PinKind::Output);
		ImGui::Text("OUT");
		ed::EndPin();
		ed::EndNode();

		if (isActive)
		{
			ed::PopStyleColor();
			ed::PopStyleVar();
		}

		// 色をもとに戻す
		if (node.type == NodeType::SubGraph)
		{
			ed::PopStyleColor();
		}
	}

	// リンク描画
	for (auto& link : currentGraph.links)
	{
		ed::Link(link.linkId, link.startPin, link.endPin, link.color);
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
				"Stamina",
				"Avoid",
				"AnimationSpeed",
			};
			int actionIndex = (int)action.type;
			if (ImGui::Combo("Action", &actionIndex, actionTypes, IM_ARRAYSIZE(actionTypes)))
				action.type = (TransitionActionType)actionIndex;

			switch (action.type)
			{
			case TransitionActionType::MoveSpeed:
			case TransitionActionType::TurnSpeed:
			case TransitionActionType::ConsumeStamina:
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

			case TransitionActionType::SetAnimationSpeed:
			{
				float AnimSpeed = ImGui::DragFloat("Amout", &action.value, 0.01f, 0.0f, 10.0f);
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

			// 条件タイプのドロップダウン
			const char* condTypes[] = {
				"AnimationFinished",	// アニメーションが終了したら
				"AnimationTimeOver",	// 指定の再生時間オを過ぎたら
				"AnimationTimeIn",		// 指定の再生時間内だったら
				"ButtonPressed",		// ボタンを押した瞬間
				"ButtonReleased",		// ボタンを離した瞬間
				"ButtonHeld",			// ボタンを長押ししているとき
				"MoveLengthOver",		// 入力が一定以上の時
				"MoveLengthUnder",		// 入力が一定以下の時
				"BHold",				// Bボタンを長押している時
				"BTap",					// Bボタンを単押ししている時
				"RBTap",				// RBボタンを単押ししている時
				"RTHold",				// RTボタンを長押している時
				"RTTap",				// RTボタンを単押ししている時
				"JumpPressed",			// ジャンプ可能かどうか
				"StaminaEmpty",			// スタミナが空かどうか
				"HasStamina",			// スタミナを持っているか
				"IsLockOn",				// ロックオン中かどうか
				"CanRun",				// 走れるかどうか
				"HavePotion",			// ポーションを持っているか
				"HealCooldownReady",	// 回復のクールダウン中か
				"IsGuarding",			// ガード中か
				"PoaitionX",			// X軸が～なら
				"PoaitionY",			// Y軸が～なら
				"PoaitionZ",			// Z軸が～なら
				"Always",				// 無条件
			};
			int typeIdx = (int)cond.type;
			if (ImGui::Combo("Type", &typeIdx, condTypes, IM_ARRAYSIZE(condTypes)))
				cond.type = (TransitionConditionType)typeIdx;

			// タイプによって追加入力欄を出す
			switch (cond.type)
			{
			case TransitionConditionType::AnimationTimeOver:
			case TransitionConditionType::AnimationTimeIn:
			case TransitionConditionType::MoveLengthOver:
			case TransitionConditionType::MoveLengthUnder:
				ImGui::DragFloat("Threshold", &cond.threshold, 0.01f, 0.0f, 10.0f);
				break;

			case TransitionConditionType::ButtonPressed:
			case TransitionConditionType::ButtonReleased:
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
				int currentIndex = 0;
				for (int i = 0; i < IM_ARRAYSIZE(buttonMasks); i++)
				{
					if (cond.buttonMask == buttonMasks[i]) { currentIndex = i; break; }
				}
				if (ImGui::Combo("Button", &currentIndex, buttonNames, IM_ARRAYSIZE(buttonNames)))
					cond.buttonMask = buttonMasks[currentIndex];
				break;
			}
			case TransitionConditionType::BHold:
			case TransitionConditionType::BTap:
			case TransitionConditionType::RBTap:
			case TransitionConditionType::RTHold:
			case TransitionConditionType::RTTap:
			case TransitionConditionType::JumpPressed:
				ImGui::TextDisabled(u8"（追加設定なし）");
				break;

			case TransitionConditionType::PositionX:
			case TransitionConditionType::PositionY:
			case TransitionConditionType::PositionZ:
				ImGui::DragFloat("Thershold", &cond.threshold, 0.01f, 0.0f, 1000.0f);
				break;
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

		// ノードの種類を切り替えるボタン
		if (ImGui::Button(selectedNode->type == NodeType::Animation ? u8"階層(SubGraph)に変換" : u8"通常(Animation)に変換"))
		{
			selectedNode->type = (selectedNode->type == NodeType::Animation) ? NodeType::SubGraph : NodeType::Animation;
		}

		ImGui::Separator();

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

			// ブレンド時間
			ImGui::DragFloat(u8"ブレンド時間(秒)", &selectedNode->config.blendTime, 0.01f, 0.0f, 2.0f);
		}
	}
	ImGui::End();
}
