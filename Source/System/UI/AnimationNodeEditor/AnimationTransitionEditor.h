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
	void Draw(std::vector< AnimationTransitionGraph>& graphs);

private:
	// リンク選択
	void DrawSelectedLinkEditor(AnimationTransitionGraph& graph);

	ed::EditorContext* context = nullptr;

    int currentGraphIndex = 0; // 今選択中のグラフ番号
    bool showSelectNodeWindow = false;
    bool needAutoLayout = false;
};
