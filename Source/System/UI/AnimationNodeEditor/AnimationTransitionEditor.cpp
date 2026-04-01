#include "AnimationTransitionEditor.h"
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"

// ノードエディタ描画
void AnimationTransitionEditor::Draw(AnimationTransitionGraph& graph)
{
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

	// リンク削除
	if (ed::BeginDelete())
	{
		ed::LinkId deletedLink;
		if (ed::QueryDeletedLink(&deletedLink));
		{
			if (ed::AcceptDeletedItem())
				graph.RemoveLink(deletedLink);
		}
	}
	ed::EndDelete();

	ed::End();

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
		ImGui::Text("From State: %d -> To State: %d", trans.fromState, trans.toState);
		ImGui::DragInt("Priority", &trans.priority);
		ImGui::Separator();

		// 条件リストを表示
		for (int i = 0; i < (int)trans.conditions.size(); i++)
		{
			TransitionCondition& cond = trans.conditions[i];
			ImGui::PushID(i);

			// 条件タイプのドロップダウン
			const char* condTypes[] = {
				"AnimationFinished", // アニメション終了
				"AnimationTimeOver", // 指定秒数を超えたら
				"AnimationTimeIn", // 指定秒数以内であれば
				"ButtonPressed", // ボタンを押した瞬間
				"ButtonReleased", // ボタンを離した瞬間
				"ButtonHeld", // ボタン長押し
				"MoveLengthOver", // 移動値が一定以上
				"MoveLengthUnder", // 移動値が一定以下
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
				// よく使うボタンだけ並べる
				const char* buttonNames[] = {
					"A", "B", "X", "Y", "LB", "RB", "LT", "RT",
					"B_Hold", "B_Tap", "RT_Hold", "RT_Tap", "Jump"
				};
				// buttonMaskをインデックスに変換して表示
				ImGui::InputInt("ButtonMask", &cond.buttonMask);
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
	}

	ImGui::End();
}