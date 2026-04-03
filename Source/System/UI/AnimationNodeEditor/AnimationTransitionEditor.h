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
	void Draw(AnimationTransitionGraph& graph);

private:
	// リンク選択
	void DrawSelectedLinkEditor(AnimationTransitionGraph& graph);

	ed::EditorContext* context = nullptr;
    bool showSelectNodeWindow = false;
    bool needAutoLayout = false;
};
