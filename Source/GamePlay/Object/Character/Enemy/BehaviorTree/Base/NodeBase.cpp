#include "NodeBase.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/JudgmentBase.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorData.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/ActionBase.h"

// デストラクタ
NodeBase::~NodeBase()
{
    delete judgment;
    delete action;
}

// ノード検索
NodeBase* NodeBase::SearchNode(std::string searchName)
{
    // 名前が一致
    if (name == searchName)
    {
        return this;
    }
    else {
        // 子ノードで検索
        for (auto itr = children.begin(); itr != children.end(); itr++)
        {
            NodeBase* ret = (*itr)->SearchNode(searchName);

            if (ret != nullptr)
            {
                return ret;
            }
        }
    }

    return nullptr;
}

// ノード推論
NodeBase* NodeBase::Inference(BehaviorData* data)
{
    std::vector<NodeBase*> list;
    NodeBase* result = nullptr;

    // childrenの数だけループを行う
    for (int i = 0; i < children.size(); i++)
    {
        // children.at(i)->judgmentがnullptrでなければ
        if (children.at(i)->judgment != nullptr)
        {
            // children.at(i)->judgment->Judgment()関数を実行し、trueであれば
            // listにchildren.at(i)を追加していく
            if (children.at(i)->judgment->Judgment())
            {
                list.push_back(children.at(i));
            }
        }
        else {
            // 判定クラスが無ければ無条件に追加
            list.push_back(children.at(i));
        }
    }

    // 選択ルールでノード決め
    switch (selectRule)
    {
        // 優先順位
    case BehaviorTree::SelectRule::Priority:
        result = SelectPriority(&list);
        break;
        // ランダム
    case BehaviorTree::SelectRule::Random:
        result = SelectRandom(&list);
        break;
        // シーケンス
    case BehaviorTree::SelectRule::Sequence:
    case BehaviorTree::SelectRule::SequentialLooping:
        result = SelectSequence(&list, data);
        break;
    }

    if (result != nullptr)
    {
        // 行動があれば終了
        if (result->HasAction() == true)
        {
            return result;
        }
        else {
            // 決まったノードで推論開始
            result = result->Inference(data);
        }
    }

    return result;
}

// 優先順位でノード選択
NodeBase* NodeBase::SelectPriority(std::vector<NodeBase*>* list)
{
    if (list->empty()) return nullptr;

    NodeBase* selectNode = nullptr;
    int priority = INT_MAX;

    // 一番優先順位が高いノードを探してselectNodeに格納
    selectNode = (*list)[0];
    for (auto node : *list)
    {
        if (node->GetPriority() < selectNode->GetPriority())
        {
            selectNode = node;
        }
    }

    return selectNode;
}

// ランダムでノードを選択
NodeBase* NodeBase::SelectRandom(std::vector<NodeBase*>* list)
{
    int selectNo = 0;
    // listのサイズで乱数を取得「してselectNoに格納
    if (list->empty()) return nullptr;

    selectNo = rand() % list->size();

    // listのselectNo番目の実体をリターン
    return (*list).at(selectNo);
}

// シーケンス・シーケンシャルルーピングでノード選択
NodeBase* NodeBase::SelectSequence(std::vector<NodeBase*>* list, BehaviorData* data)
{
    int step = 0;

    // 指定されている中間ノードのシーケンスがどこまで実行されたか取得する
    step = data->GetSequenceStep(name);

    // 中間ノードに登録されているノードづう以上の場合
    if (step >= children.size())
    {
        // ルールによって処理を切り替える
        // ルールがBehaviorTree::SelectRule::SequentialLoopingのときは最初から実行するため、stepに0を代入
        // ルールがBehaviorTree::SelectRule::Sequenceのときは次に実行できるノードがないため、nullptrをリターン
        if (selectRule == BehaviorTree::SelectRule::Sequence)
        {
            return nullptr;
        }
        else if (selectRule == BehaviorTree::SelectRule::Sequence)
        {
            return nullptr;
        }
        else if (selectRule == BehaviorTree::SelectRule::SequentialLooping)
        {
            step = 0;
            data->SetSequenceStep(name, 0);
        }
        // 実行可能リストに登録されているデータの数だけループを行う
        for (auto itr = list->begin(); itr != list->end(); itr++)
        {
            // 現在の実行ノードの保存、次に実行するステップの保存を行った後、
            // 現在のステップ番号のノードをリターンしなさい
            // ①スタックにはdata->PushSequenceNode関数を使用する。保存するデータは実行中の中間ノード。
            // ②また、次に実行する中間ノードとステップ数を保存する
            // 　保存にはdata->SetSequenceStep関数を使用。
            // 　保存データは中間ノードの名前と次のステップ数です(step + 1)
            // ③ステップ番号目の子ノードを実行ノードとしてリターンする
            data->PushSequenceNode(this);

            data->SetSequenceStep(name, step + 1);

            return (*list)[step];
        }
    }
    // 指定された中間ノードに実行可能ノードが無いのでnullptrをリターンする
    return nullptr;
}

// 判定
bool NodeBase::Judgment()
{
    // judgmentがあるか判断。あればメンバ関数Judgment()実行した結果をリターン。
    if (judgment == nullptr)
    {
        return true;
    }
    else
    {
        return judgment->Judgment();
    }
}

// ノード実行
ActionBase::State NodeBase::Run(float elapsedTime)
{
    // actionがあるか判断。あればメンバ関数Run()実行した結果をリターン。
    if (action == nullptr)
    {
        return ActionBase::State::Failed;
    }
    else
    {
        return action->Run(elapsedTime);
    }
}
