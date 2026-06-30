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
    std::string Draw(std::vector<AnimationTransitionGraph>& graphs,
        const std::string& activeState = "",
        const std::vector<int>& activeGraphStack = {});
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
