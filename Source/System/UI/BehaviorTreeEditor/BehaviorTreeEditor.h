#pragma once
#include "System/UI/BehaviorTreeEditor/BehaviorTreeGraph.h"

class BehaviorTreeRunner;

class BehaviorTreeEditor
{
public:
	BehaviorTreeEditor() { context = ed::CreateEditor(); }
	~BehaviorTreeEditor() { ed::DestroyEditor(context); }

	void Draw(BehaviorTreeGraph& graph, BehaviorTreeRunner& runner, Enemy* enemy);
	void SetJsonPath(const std::string& path) { jsonPath = path; }

	void SetAnimationList(const std::vector<std::string>& list)
	{
		animationNames = list;
	}

	void ApplyToTree(BehaviorTree* tree, Enemy* enemy, const BehaviorTreeGraph& graph);

	bool requestOpenSequencer = false;
	std::string selectedAnimName = "";
private:
	void DrawToolbar(BehaviorTreeGraph& graph, BehaviorTreeRunner& runner, Enemy* enemy);
	void DrawInspector(BehaviorTreeGraph& graph);

private:
	ed::EditorContext* context = nullptr;
	int selectedNodeIndex = -1;

	std::vector<std::string> animationNames;
	std::string jsonPath;

	bool isNeedRebuild = false;
};