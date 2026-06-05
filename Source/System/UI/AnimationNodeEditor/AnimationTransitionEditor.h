#pragma once
#include "AnimationTransitionGraph.h"

class AnimationTransitionEditor
{
public: 
    AnimationTransitionEditor()
    {
        context = ed::CreateEditor();
    }

    ~AnimationTransitionEditor()
    {
        ed::DestroyEditor(context);
    }

	// ノードエディタ描画
	//void Draw(AnimationTransitionGraph& graph);
	//void Draw(std::vector< AnimationTransitionGraph>& graph);
	std::string Draw(std::vector< AnimationTransitionGraph>& graph, const std::string& activeState = "");

private:
	// リンク選択
	void DrawSelectedLinkEditor(AnimationTransitionGraph& graph);

    // ノード選択
    void DrawSelectedNodeEditor(AnimationTransitionGraph& graph);

	ed::EditorContext* context = nullptr;
    AnimLink* copiedLink = nullptr; // コピー元のリンクを保持する変数

    int currentGraphIndex = 0; // 今選択中のグラフ番号
    bool showSelectNodeWindow = false;
    bool needAutoLayout = false;
    bool OpenCopyMenu = false;

    // 今どのグラフの階層にいるかを記憶するスタック
    std::vector<int> graphStack;
};
