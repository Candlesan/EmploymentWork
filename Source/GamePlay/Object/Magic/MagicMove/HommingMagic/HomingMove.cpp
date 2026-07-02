#include "HomingMove.h"

// コンストラクタ
HomingMove::HomingMove(DirectX::XMFLOAT3 targetPos, float turnSpeed)
	:target(targetPos), turnSpeed(turnSpeed)
{
}

// 更新処理
void HomingMove::UpdateMove(DirectX::XMFLOAT3& position, DirectX::XMFLOAT3& direction, float speed, float elapsedTime)
{
	// ターゲットとの距離を計算
	float dx = target.x - position.x;
	float dy = target.y - position.y;
	float dz = target.z - position.z;
	float dist = sqrtf(dx * dx + dy * dy + dz * dz);

	//旋回
	if (isHoming && dist < 1.0f)
	{
		isHoming = false;
	}

	if (isHoming)
	{
		float turnSpeed = this->turnSpeed * elapsedTime;

		//ターゲットまでのベクトルを算出
		DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR Target = DirectX::XMLoadFloat3(&target);
		DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Target, Position);

		//ゼロベクトルでないなら回転処理
		DirectX::XMVECTOR LengthSq = DirectX::XMVector3Length(Vec);
		float lengthSq;
		DirectX::XMStoreFloat(&lengthSq, LengthSq);
		if (lengthSq > 0.001f)
		{
			//ターゲットまでのベクトルを単位ベクトル化
			Vec = DirectX::XMVector3Normalize(Vec);

			//向いている方向ベクトルを算出
			DirectX::XMVECTOR Direction = DirectX::XMLoadFloat3(&direction);

			//前方方向ベクトルとターゲットまでのベクトルの内積(内積)を算出
			DirectX::XMVECTOR Dot = DirectX::XMVector3Dot(Direction, Vec);

			float dot;
			DirectX::XMStoreFloat(&dot, Dot);

			//2つの単位ベクトルの角度が小さいほど
			//1.0fに近づくという性質を利用して回転速度を調整する
			float rot = 1.0 - dot; //補正値
			if (rot > turnSpeed) rot = turnSpeed;

			//回転速度があるなら回転処理をする
			if (fabsf(rot) > 0.0001f)
			{
				Direction = DirectX::XMLoadFloat3(&direction);

				//回転軸を算出
				DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(Direction, Vec);
				Axis = DirectX::XMVector3Normalize(Axis);

				//回転軸と回転量から回転行列を算出
				DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationAxis(Axis, rot);

				Direction = DirectX::XMVector3TransformNormal(Direction, Rotation);

				Direction = DirectX::XMVector3Normalize(Direction);
				DirectX::XMStoreFloat3(&direction, Direction);
			}
		}
	}

	// 移動
	float moveDist = speed * elapsedTime;
	position.x += direction.x * moveDist;
	position.y += direction.y * moveDist;
	position.z += direction.z * moveDist;
}
