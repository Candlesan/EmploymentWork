#pragma once
class BehaviorTree;
class BehaviorData;
class NodeBase;

// ビヘイビアツリー1本分の実行状態
class BehaviorTreeRunner
{
public:
    BehaviorTreeRunner();
    ~BehaviorTreeRunner(); // tree と data を delete する

    void Update(float elapsedTime); // 推論・中断・実行
    void Abort();                   // 実行中のノードを止める（全身行動のとき用）
    void Reset();

    BehaviorTree* GetTree() { return tree; }
    NodeBase* GetActiveNode() const { return activeNode; }
    NodeBase* GetLastInferredNode() const { return lastInferredNode; }

private:
    BehaviorTree* tree = nullptr;
    BehaviorData* data = nullptr;
    NodeBase* activeNode = nullptr;
    NodeBase* lastInferredNode = nullptr;
};