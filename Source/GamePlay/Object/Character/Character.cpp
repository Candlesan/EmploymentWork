#include "Gameplay/Object/Character/Character.h"

void Character::UpdateTransform()
{
	//スケール行列作成
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	//回転行列を作成
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	//位置行列を作成
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y, position.z);

	//3つの行列を組み合わせ,ワールド行列を作成
	DirectX::XMMATRIX W = S * R * T;

	//計算したワールド行列を取り出す
	DirectX::XMStoreFloat4x4(&transform, W);
}

//移動処理
void Character::Move(float vx, float vz, float speed)
{
	//移動方向ベクトル
	moveVecX = vx;
	moveVecZ = vz;

	//最大速度設定
	maxMoveSpeed = speed;
}

//旋回処理
void Character::Turn(float elapsedTime, float vx, float vz, float speed)
{
	speed *= elapsedTime;

	//進行ベクトルがゼロベクトルの場合は処理をする必要なし
	float length = sqrtf(vx * vx + vz * vz);
	if (length < 0.001f) return;

	//進行ベクトルを単位ベクトル化
	vx /= length;
	vz /= length;

	//自身の回転値から前方向を求める
	float frontX = sinf(angle.y);
	float frontZ = cosf(angle.y);

	//回転地を求めるため、２つの単位ベクトルの内積を計算する
	float dot = (frontX * vx) + (frontZ * vz);

	//内積は-1.0~1.0で表現されており、２つの単位ベクトルの角度が
	//小さいほど1.0に近づくという性質を利用して回転速度を調整する
	float rot = 1.0 - dot; //補正
	if (rot > speed) rot = speed;

	//左右判定を行うために2つの単位ベクトルの外積を計算する
	float cross = (frontZ * vx) - (frontX * vz);

	//2Dの外積値が正の場合か負の場合によって左右判定が行える
	//左右判定を行うことによって左右回転を選択する
	if (cross < 0.0f)
	{
		angle.y -= rot;
	}
	else
	{
		angle.y += rot;
	}
}

//ジャンプ処理
void Character::Jump(float speed)
{
	velocity.y += speed;
}

//速力処理更新
void Character::UpdateVelocity(float elapsedTime)
{
	//水平速力更新処理
	UpdateHorizontalVelocity(elapsedTime);

	//水平移動更新処理
	UpdateHorizontalMove(elapsedTime);

	//垂直速力更新処理
	UpdateVerticalVelocity(elapsedTime);

	//垂直移動更新処理
	UpdateVerticalMove(elapsedTime);
}

// 無敵時間更新
void Character::UpdateInvincibleTimer(float elapsedTime)
{
	if (invincibleTimer > 0.0f)
	{
		invincibleTimer -= elapsedTime;
	}
}

//ダメージを与える
bool Character::ApplyDamage(float damage, float invincibleTime)
{
	// ダメージが0の場合は体力を変更しない
	if (damage <= 0) return false;

	// 死亡している場合は体力を変更しない
	if (health <= 0) return false;

	// 無敵時間中はダメージを受けない
	if (invincibleTimer > 0.0f) return false;

	// 無敵時間設定
	invincibleTimer = invincibleTime;

	// ダメージ処理
	health -= damage;

	// 死亡通知 
	if (health <= 0)
	{
		OnDead();
	}
	// ダメージ通知
	else
	{
		OnDamage();
	}

	// 体力が変更した場合はtrueを返す
	return true;
}

//水平速力更新処理
void Character::UpdateHorizontalVelocity(float elapsedTime)
{
	//移動処理
	position.x += velocity.x * elapsedTime;
	position.z += velocity.z * elapsedTime;
}

//水平移動更新処理
void Character::UpdateHorizontalMove(float elapsedTime)
{
	//XZ平面の速力を減速
	float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (length > 0.0f)
	{
		//摩擦力
		float friction = this->friction * elapsedTime;

		//摩擦による横方向の減速処理
		if (length > friction)
		{
			//単位ベクトル化
			float vx = velocity.x / length;
			float vz = velocity.z / length;
			//速力を減らす
			velocity.x -= vx * friction;
			velocity.z -= vz * friction;
		}
		//横方向の速力が摩擦力以下になってたので速力を無効化
		else
		{
			velocity.x = 0.0f;
			velocity.z = 0.0f;
		}
	}

	//XZ平面の速力を加速する
	if (length <= maxMoveSpeed)
	{
		//移動ベクトルがゼロベクトルでないなら加速する
		float moveVeclength = sqrtf(moveVecX * moveVecX + moveVecZ * moveVecZ);
		if (moveVeclength > 0.0f)
		{
			//加速力
			float acceleration = this->acceleration * elapsedTime;

			//移動ベクトルによる加速処理
			velocity.x += acceleration * moveVecX;
			velocity.z += acceleration * moveVecZ;

			//最大速度制限
			float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
			if (length > maxMoveSpeed)
			{
				//速度を単位ベクトル化
				float vx = velocity.x / length;
				float vz = velocity.z / length;
				//速度設定
				velocity.x = vx * maxMoveSpeed;
				velocity.z = vz * maxMoveSpeed;
			}
		}
	}
	//移動ベクトルをリセット
	moveVecX = 0.0f;
	moveVecZ = 0.0f;

}

//垂直速力更新処理
void Character::UpdateVerticalVelocity(float elapsedTime)
{
	//重力処理
	velocity.y += gravity * elapsedTime;
}

//垂直移動更新処理
void Character::UpdateVerticalMove(float elapsedTime)
{
	//移動処理
	position.y += velocity.y * elapsedTime;

	//地面判定
	if (position.y < 0.0f)
	{
		position.y = 0.0f;
		velocity.y = 0.0f;

		//着地した
		if (!isGround)
		{
			OnLanding();
			isGround = true;
		}
		else
		{
			isGround = false;
		}
	}
}
