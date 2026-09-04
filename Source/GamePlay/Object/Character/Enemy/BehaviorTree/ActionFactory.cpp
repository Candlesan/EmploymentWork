#include "ActionFactory.h"
#include "ActionDerived.h"
#include "System/UI/BehaviorTreeEditor/BehaviorTreeGraph.h"


ActionBase* ActionFactory::Creat(BTActionType type, Enemy* enemy, const std::string& stateName)
{
	switch (type)
	{
	case BTActionType::None:
		return nullptr;

		break;
	case BTActionType::Idle:
		return new IdleAction(enemy, stateName);

		break;
	case BTActionType::Pursuit:
		return new PursuitAction(enemy, stateName);

		break;
	case BTActionType::NormalMagicAttack:
		return new NormalMagicAttackAction(enemy, stateName);

		break;
	}
}
