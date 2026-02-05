#include"JudgmentDerive.h"
#include"Mathf.h"

//MoveNodeに移動できるか判定
bool MoveJudgment::Judgment()
{
	//目的地までの距離を判定
	DirectX::XMFLOAT3 position = owner->GetPosition();
	DirectX::XMFLOAT3 targetPosition = owner->GetTargetPosition();
	float vx = targetPosition.x - position.x;
	float vz = targetPosition.z - position.z;
	float distSq = vx * vx + vz * vz;

	//目的地から遠ければ
	float radius = owner->GetRadius();
	if (distSq > radius * radius)
	{
		return true;
	}

	return false;
}

//STOPNodeに移動できるか判定
//bool StopJudgment::Judgment()
//{
//
//	return false;
//}