#include "AnimationTransitionEditor.h"
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"
#include "System/Core/Input/Input.h"

// ノードエディタ描画
void AnimationTransitionEditor::Draw(AnimationTransitionGraph& graph)
{
	// Jsonに保存する
	if (ImGui::Button("Save"))
	{
		graph.Save("Data/Json/Player/AnimationState/AnimationState.json");
	}

	ImGui::SameLine();

	// Jsonを読み込む
	if (ImGui::Button("Load"))
	{
		graph.Load("Data/Json/Player/AnimationState/AnimationState.json");
	}

	ImGui::Separator();

	// ノードの追加
	if (ImGui::Button("+ Add Node"))
	{
		showSelectNodeWindow = true; // 追加ノード選択画面を開く
	}

	if (ImGui::Button("Auto Layout"))
	{
		needAutoLayout = true; // フラグを立てるだけ
	}

	if (showSelectNodeWindow)
	{
		ImGui::Begin("SelectNode");

		// AnimationStateManagerを取得
		auto& manager = AnimationStateManager<PlayerAnimationState>::Instance();

		for (auto& [state, config] : manager.GetAllConfigs())
		{
			// 既に追加済みのアニメーションかをチェックする
			bool alreadyAnimation = false;
			for (auto& node : graph.nodes)
			{
				if (node.animState == (int)state)
				{
					alreadyAnimation = true;
					break;
				}
			}
			if (alreadyAnimation) continue; // 追加済みはスキップする

			if (ImGui::Selectable(config.animationName.c_str()))
			{
				graph.AddNode((int)state, { 100, 100 }); // 選んだらノード追加
				showSelectNodeWindow = false; // 選んだら閉じる
			}
		}

		ImGui::End();
	}

	ed::SetCurrentEditor(context);
	ed::Begin("Animation Transition Editor");

	// ノード描画
	for (auto& node : graph.nodes)
	{
		ed::BeginNode(node.nodeId);

		AnimationStateManager<PlayerAnimationState>& stateManager = AnimationStateManager<PlayerAnimationState>::Instance();
		const AnimationConfig* config = stateManager.GetConfig(static_cast<PlayerAnimationState>(node.animState));
		ImGui::Text("State: %s", config->animationName.c_str());

		ed::BeginPin(node.pinIn, ed::PinKind::Input);
		ImGui::Text("IN");
		ed::EndPin();

		ImGui::SameLine();

		ed::BeginPin(node.pinOut, ed::PinKind::Output);
		ImGui::Text("OUT");
		ed::EndPin();
		ed::EndNode();
	}

	// リンク描画
	for (auto& link : graph.links)
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
				graph.AddLink(startPin, endPin);
		}
	}
	ed::EndCreate();

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

	ed::End();

	// 自動整列
	if (needAutoLayout)
	{
		graph.AutoLayout();
		needAutoLayout = false;
	}

	DrawSelectedLinkEditor(graph);
	ed::SetCurrentEditor(nullptr);
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
		const AnimationConfig* fromStateName = AnimationStateManager<PlayerAnimationState>
			::Instance().GetConfig(static_cast<PlayerAnimationState>(trans.fromState));

		const AnimationConfig* toStateName = AnimationStateManager<PlayerAnimationState>
			::Instance().GetConfig(static_cast<PlayerAnimationState>(trans.toState));

		ImGui::Text("From State: %s -> To State: %s", fromStateName->animationName.c_str(), toStateName->animationName.c_str());
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
			};
			int actionIndex = (int)action.type;
			if (ImGui::Combo("Action", &actionIndex, actionTypes, IM_ARRAYSIZE(actionTypes)))
				action.type = (TransitionActionType)actionIndex;

			switch (action.type)
			{
			case TransitionActionType::MoveSpeed:
			case TransitionActionType::TurnSpeed:
			case TransitionActionType::ConsumeStamina:
				ImGui::DragFloat("Amout", &action.value, 0.1f, 0.0, 1000.0);
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

			// 条件タイプのドロップダウン
			const char* condTypes[] = {
				"AnimationFinished", // アニメーションが終了したら
				"AnimationTimeOver", // 指定の再生時間オを過ぎたら
				"AnimationTimeIn", // 指定の再生時間内だったら
				"ButtonPressed", // ボタンを押した瞬間
				"ButtonReleased", // ボタンを離した瞬間
				"ButtonHeld", // ボタンを長押ししているとき
				"MoveLengthOver", // 入力が一定以上の時
				"MoveLengthUnder", // 入力が一定以下の時
				"BHold", // Bボタンを長押している時
				"BTap", // Bボタンを単押ししている時
				"RTHold", // RTボタンを長押している時
				"RTTap", // RTボタンを単押ししている時
				"JumpPressed", // ジャンプ可能かどうか
				"StaminaEmpty", // スタミナが空かどうか
				"HasStamina", // スタミナを持っているか
				"IsLockOn", // ロックオン中かどうか
				"CanRun", // 走れるかどうか
				"HavePotion",// ポーションを持っているか
				"HealCooldownReady", // 回復のクールダウン中か
				"IsGuarding", // ガード中か
				"PoaitionX", // X軸が～なら
				"PoaitionY", // Y軸が～なら
				"PoaitionZ", // Z軸が～なら
				"Always", // 無条件
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