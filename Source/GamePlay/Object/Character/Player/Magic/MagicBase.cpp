#include "GamePlay/Object/Character/Player/Magic/MagicBase.h"
#include "GamePlay/Object/Character/Player/Magic/MagicManager.h"

// コンストラクタ
MagicBase::MagicBase(MagicManager* manager)
	: manager(manager)
{
	// 生成時にマネージャーに登録する
	manager->Register(this);
}

// 破棄
void MagicBase::Destroy()
{
	// マネージャーから自分自身を削除する
	manager->Remove(this);
}

void MagicBase::UpdateTransform()
{
	DirectX::XMVECTOR Front, Up, Right;

	//前ベクトルを算出
	Front = DirectX::XMLoadFloat3(&direction);
	Front = DirectX::XMVector3Normalize(Front);

	//仮の上ベクトルを算出
	Up = DirectX::XMVectorSet(0.001f, 1, 0, 0);
	Up = DirectX::XMVector3Normalize(Up);

	//右ベクトルを算出
	Right = DirectX::XMVector3Cross(Up, Front);
	Right = DirectX::XMVector3Normalize(Right);

	//上ベクトルを算出
	Up = DirectX::XMVector3Cross(Right, Front);

	//計算結果を取り出し
	DirectX::XMFLOAT3 right, up, front;
	DirectX::XMStoreFloat3(&right, Right);
	DirectX::XMStoreFloat3(&up, Up);
	DirectX::XMStoreFloat3(&front, Front);

	//とりあえず、仮で回転を無視した行列を作成する
	transform._11 = right.x * scale.x;
	transform._12 = right.y * scale.y;
	transform._13 = right.z * scale.z;
	transform._14 = 0.0f;

	transform._21 = up.x * scale.x;
	transform._22 = up.y * scale.y;
	transform._23 = up.z * scale.z;
	transform._24 = 0.0f;

	transform._31 = front.x * scale.x;
	transform._32 = front.y * scale.y;
	transform._33 = front.z * scale.z;
	transform._34 = 0.0f;

	transform._41 = position.x;
	transform._42 = position.y;
	transform._43 = position.z;
	transform._44 = 1.0f;

	//発射方向
	this->direction = front;
}