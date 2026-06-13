#pragma once
#include "System/UI/BehaviorTreeEditor/BehaviorTreeGraph.h"

class BehaviorTreeEditor
{
public:
	BehaviorTreeEditor() { context = ed::CreateEditor(); }
	~BehaviorTreeEditor() { ed::DestroyEditor(context); }

	void Draw(BehaviorTreeGraph& graph);

	void SetAnimationList(const std::vector<std::string>& list)
	{
		animationNames = list;
	}

	void ApplyToTree(BehaviorTree* tree, Enemy* enemy, const BehaviorTreeGraph& graph);

private:
	void DrawToolbar(BehaviorTreeGraph& graph);
	void DrawInspector(BehaviorTreeGraph& graph);

	// ƒsƒ“‚Ì•`‰æƒwƒ‹ƒp[ŠÖ”
	void DrawArrowPin(bool filled);

	ed::EditorContext* context = nullptr;
	int selectedNodeIndex = -1;

	std::vector<std::string> animationNames; 
};