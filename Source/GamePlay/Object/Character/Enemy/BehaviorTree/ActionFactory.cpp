#include "ActionFactory.h"
#include "ActionDerived.h"
#include "System/UI/BehaviorTreeEditor/BehaviorTreeGraph.h"


ActionBase* ActionFactory::Creat(BTActionType type, Enemy* enemy, const std::string& stateName)
{
	// ファクトリーでアクションを作る
	switch (type)
	{
	case BTActionType::None:
		return nullptr;

		break;
	case BTActionType::Idle:
		return new Idle(enemy, stateName);

		break;
	case BTActionType::MoveForward:
		return new MoveForward(enemy, stateName);

		break;
	case BTActionType::MoveBack:
		return new MoveBack(enemy, stateName);

		break;
	case BTActionType::Right:
		return new MoveRight(enemy, stateName);

		break;
	case BTActionType::Left:
		return new MoveLeft(enemy, stateName);

		break;
	case BTActionType::NormalMagicAttack:
		return new NormalMagicAttack(enemy, stateName);

		break;

	case BTActionType::RapidFire:
		return new RapidFireMagic(enemy, stateName);

		break;
	case BTActionType::Intercept:
		return new InterceptMagic(enemy, stateName);

		break;
	case BTActionType::Guard:
		return new GuardMagic(enemy, stateName);

		break;
	case BTActionType::DropGuard:
		return new DropGuardMagic(enemy, stateName);

		break;
	case BTActionType::Avoid:
		return new Avoid(enemy, stateName);

		break;
	case BTActionType::Flight:
		return new Flight(enemy, stateName);

		break;
	case BTActionType::Land:
		return new Land(enemy, stateName);

		break;
	}

	// 当てはまるものが無かったらnullptrを返す
	return nullptr;
}
