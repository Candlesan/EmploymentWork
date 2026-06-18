#pragma once
#include "System/UI/BehaviorTreeEditor/BehaviorTreeGraph.h"

class BehaviorTreeEditor
{
public:
	BehaviorTreeEditor() { context = ed::CreateEditor(); }
	~BehaviorTreeEditor() { ed::DestroyEditor(context); }

	void Draw(BehaviorTreeGraph& graph, BehaviorTree*& tree, Enemy* enemy);

	void SetAnimationList(const std::vector<std::string>& list)
	{
		animationNames = list;
	}

	void ApplyToTree(BehaviorTree* tree, Enemy* enemy, const BehaviorTreeGraph& graph);

	bool requestOpenSequencer = false;
	std::string selectedAnimName = "";
private:
	void DrawToolbar(BehaviorTreeGraph& graph, BehaviorTree*& tree, Enemy* enemy);
	void DrawInspector(BehaviorTreeGraph& graph);

	// ÉsÉìÇÃï`âÊÉwÉãÉpÅ[ä÷êî
	void DrawArrowPin(bool filled);

	ed::EditorContext* context = nullptr;
	int selectedNodeIndex = -1;

	std::vector<std::string> animationNames; 
};