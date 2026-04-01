#include "AnimationTransitionGraph.h"
#include <algorithm>

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

// 矢印を消す
void AnimationTransitionGraph::RemoveLink(ed::LinkId id)
{
	// find_if:イテレーターの範囲内から指定された条件を満たす最初の要素を検索する関数
	auto it = std::find_if(links.begin(), links.end(),
		[&](const AnimLink& l) { return l.linkId == id; });
}

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
			result = !ctx.isStaminaEmpty;
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
