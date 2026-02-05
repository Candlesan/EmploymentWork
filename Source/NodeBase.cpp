#include"JudgmentBase.h"
#include"NodeBase.h"
#include"BehaviorData.h"
#include"ActionBase.h"

//デストラクタ
NodeBase::~NodeBase()
{
	delete judgment;
	delete action;
}

//ノード検索
NodeBase* NodeBase::SearchNode(std::string searchName)
{
	//名前が一致
	if (name == searchName)
	{
		return this;
	}
	else
	{
		//子ノードで検索
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

//ノード推論
NodeBase* NodeBase::Inference(BehaviorData* data)
{
	std::vector<NodeBase*>list;
	NodeBase* result = nullptr;

	//childrenの数だけループ
	for (int i = 0; i < children.size(); i++)
	{
		//childrenにjudgmentがあるなら
		if (children.at(i)->judgment != nullptr)
		{
			//judgmentがtrueなら
			if (children.at(i)->judgment->Judgment())
			{
				list.push_back(children.at(i));
			}
		}
		else
		{
			//判定クラスがないなら無条件に追加
			list.push_back(children.at(i));
		}
	}

	//選択ルールでノード決め
	switch (selectRule)
	{
		//優先順位
	case BehaviorTree::SelectRule::Priority:
		result = SelectPriority(&list);
		break;

		//ランダム
	case BehaviorTree::SelectRule::Random:
		result = SelectRandom(&list);
		break;

		//シーケンス
	case BehaviorTree::SelectRule::Sequence:
		//シーケンスループ
	case BehaviorTree::SelectRule::SequentialLooping:
		result = SelectSequence(&list, data);
		break;

	}

	if (result != nullptr)
	{
		//行動があれば終了
		if (result->HasAction() == true)
		{
			return result;
		}
		else
		{
			//決まったノードで推論開始
			result = result->Inference(data);
		}
	}

	return result;
}

//優先順位でノード選択
NodeBase* NodeBase::SelectPriority(std::vector<NodeBase*>* list)
{
	NodeBase* selectNode = nullptr;
	int priority = INT_MAX;

	//優先順位が高いノードを探してselectNodeに格納
	for (int i = 0; i < list->size(); i++)
	{
		if ((*list)[i]->GetPriority() < priority)
		{
			priority = (*list)[i]->GetPriority();
			selectNode = (*list)[i];
		}
	}
	return selectNode;
}

//ランダムでノード選択
NodeBase* NodeBase::SelectRandom(std::vector<NodeBase*>* list)
{
	int selectNo = 0;
	
	//listのサイズを取得してselectNoに格納
	selectNo = rand() % list->size();

	//listのselectNo番面をリターン
	return (*list).at(selectNo);
}

//シーケンス・シーケンスループでノード選択
NodeBase* NodeBase::SelectSequence(std::vector<NodeBase*>* list, BehaviorData* data)
{
	int step = 0;

	//指定されている中間ノードのシーケンスがどこまで実行されたか取得
	step = data->GetSequenceStep(name);

	//中間ノードに登録されているノード以上の場合
	if (step >= children.size())
	{
		if (selectRule == BehaviorTree::SelectRule::SequentialLooping)
		{
			// ループ処理にする
			step = 0; 
			//データ更新
			data->SetSequenceStep(name, step);
		}
		else if (selectRule == BehaviorTree::SelectRule::Sequence)
		{
			return nullptr;
		}
	}

	//実行可能リストに登録されている数だけループを行う(シーケンス)
	for (auto itr = list->begin(); itr != list->end(); itr++)
	{
		//子ノードが実行可能リストに含まれているか
		if (children.at(step)->GetName() == (*itr)->GetName())
		{
			//実行中ノード保持
			data->PushSequenceNode(this);
			//次実行するステップ保持
			data->SetSequenceStep(name, step + 1);
			//対象ステップのノードを返す
			return children.at(step);
		}
	}
	//指定された中間ノードに実行可能ノードがないためnullptrを返す
	return nullptr;
}

//判定
bool NodeBase::Judgment()
{
	//judgmentがあるか判断、あればjudgmentを実行する
	if (judgment != nullptr)
	{
		return judgment->Judgment();
	}
	//judgmentがなければ無条件でtrue
	return true;
}

//ノード実行
ActionBase::State NodeBase::Run(float elapsedTime)
{
	//actionがあれば
	if (action != nullptr)
	{
		return action->Run(elapsedTime);
	}
	//中間ノードなので失敗とする
	return ActionBase::State::Failed;
}