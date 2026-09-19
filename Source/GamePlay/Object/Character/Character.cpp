#include "Gameplay/Object/Character/Character.h"


// 行列更新
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

// ダメージ処理
bool Character::ApplyDamage(float damage)
{
	// HPを減らす
	this->status.currentHp -= damage;

	// HPが0以下だったら0にする
	if (this->status.currentHp <= 0.0f) this->status.currentHp = 0.0f;

	// 死亡・ダメージ通知
	if (this->status.currentHp <= 0) this->OnDead();
	else this->OnDamage();

	return true;
}

// ステータス更新
void Character::UpdateStatus(float elapsedTime)
{
	// 魔力回復
	// 攻撃していないまたはガードしながら攻撃していないとき
	if (status.isGuarding)
	{
		status.guardMagicRecoverTimer += elapsedTime;

		// 1秒経過したら回復して、タイマーをリセット
		if (status.guardMagicRecoverTimer >= 1.0f)
		{
			status.currentMagic += status.guardMagicRecoverAmount;
			if (status.currentMagic > status.maxMagic)
				status.currentMagic = status.maxMagic;

			status.guardMagicRecoverTimer -= 1.0f; // 0にリセットせず引き算にすると誤差が溜まりにくい
		}
	}
	else
	{
		// ディレイタイマーがあるか
		if (status.magicRecoverDelayTimer > 0.0f)
		{
			status.magicRecoverDelayTimer -= elapsedTime;

			// ディレイタイマーが0いかにならないようにする/
			// ダウン中ならディレイタイマーを0にする
			if (status.magicRecoverDelayTimer <= 0.0f || status.isDown)
			{
				status.magicRecoverDelayTimer = 0;
			}

		}
		else if (status.isDown) // ダウン中は急速に回復させる
		{
			if (status.currentMagic < status.maxMagic)
			{
				//　仮で5倍の速度で回復させる
				status.currentMagicRecoverSpeed = (status.magicRecoverSpeed * 5.0f);
				status.currentMagic += status.currentMagicRecoverSpeed * elapsedTime;

				// 最大値を超えないようにする
				if (status.currentMagic > status.maxMagic)
				{
					status.currentMagic = status.maxMagic; // 最大値を超えないようにする

					status.isDown = false; // 最大値まで回復させたらフラグを折る
				}

			}
		}
		else // ディレイタイマーがなくなったら魔力を回復する
		{
			if (status.currentMagic < status.maxMagic)
			{
				status.guardMagicRecoverTimer = 0.0f; // ガードを解除したらタイマーを0にする
				status.currentMagicRecoverSpeed = status.magicRecoverSpeed;

				// 魔力を回復させる
				status.currentMagic += status.currentMagicRecoverSpeed * elapsedTime;

				// 最大値を超えないようにする
				if (status.currentMagic > status.maxMagic)
					status.currentMagic = status.maxMagic;
			}
			else
			{
				// 魔力が満タンの時は回復速度を0にしておく
				status.currentMagicRecoverSpeed = 0;
			}
		}
	}

	// ヒート値回復
	if (status.isRapidFire)
	{
		// 迎撃しながらの連射は消費が大きい
		float cost = status.isIntercept ? status.interceptMagicCost : status.rapidFireMagicCost;
		ConsumMagic(cost * elapsedTime);

		// ヒート値を回復させる
		status.currentHeat += status.heatIncreaseSpeed * elapsedTime;

		// 最大値を超えないようにする
		if (status.currentHeat > status.maxHeat)
			status.currentHeat = status.maxHeat;
	}
	else
	{
		if (status.currentHeat > 0.0f)
		{
			// ヒート値があったら下げていく
			status.currentHeat -= status.heatRecoverSpeed * elapsedTime;

			// ヒート値が0よりも低かったら
			if (status.currentHeat < 0.0f)
				status.currentHeat = 0.0f;
		}
	}

	// ガード値回復
	if (!status.isGuarding) // ガードしてないとき
	{
		// 現在の数値が最大値よりも少なかったら
		if (status.currentDefenseDurability < status.maxDefenseDurability)
		{
			status.currentDefenseDurability += status.defenseRecoverSpeed * elapsedTime;
		}

		// 現在の値が最大値を超えていたら
		if (status.currentDefenseDurability > status.maxDefenseDurability)
			status.currentDefenseDurability = status.maxDefenseDurability;
	}
	else // ガードしてるときは魔力を減らす
	{
		ConsumMagic(status.defenseMagicCost * elapsedTime); // ここでelapsedTimeを掛けてるのは秒間でどれだけ減らすかにするため
	}
}

// 魔力消費
void Character::ConsumMagic(float amount, float DelayTime)
{
	// 魔力を減らす
	status.currentMagic -= amount;

	// 0以下にならないようにする
	if (status.currentMagic <= 0.0f)
	{
		status.currentMagic = 0.0f; 
		status.isDown = true; // 0以下になったらダウンをONにする
	}

	// 遅延をセットする
	status.magicRecoverDelayTimer = DelayTime;
}

void Character::ConsumMagic(float amount)
{
	// 魔力を減らす
	status.currentMagic -= amount;

	// 0以下にならないようにする
	if (status.currentMagic <= 0.0f)
	{
		status.currentMagic = 0.0f;
		status.isDown = true; // 0以下になったらダウンをONにする
	}
}

// 魔力回復
void Character::RecoverMagic(float amount)
{
	status.currentMagic += amount;

	// 最大値を超えないようにする
	if (status.currentMagic >= status.maxMagic)
		status.currentMagic = status.maxMagic;
}

// ガード耐久値減少
void Character::OnGuardHit(float amount)
{
	status.currentDefenseDurability -= amount;

	if (status.currentDefenseDurability < 0.0f)
		status.currentDefenseDurability = 0.0f;
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
		// 空中にいる時は摩擦をほぼ0にする
		float currentFriction = isGround ? (this->friction * elapsedTime) : (this->friction * 0.02f * elapsedTime);

		//摩擦力
		//float friction = this->friction * elapsedTime;

		//摩擦による横方向の減速処理
		if (length > currentFriction)
		{
			//単位ベクトル化
			float vx = velocity.x / length;
			float vz = velocity.z / length;
			//速力を減らす
			velocity.x -= vx * currentFriction;
			velocity.z -= vz * currentFriction;
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
			// 空中でスティックを倒したときの空中制御力を下げる。自由にターンできるのを防ぐため
			float currentAcceleration = isGround ? (this->acceleration * elapsedTime) : (this->acceleration * 0.3f * elapsedTime);

			//加速力
			//float acceleration = this->acceleration * elapsedTime;

			//移動ベクトルによる加速処理
			velocity.x += currentAcceleration * moveVecX;
			velocity.z += currentAcceleration * moveVecZ;

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
	}
	else
	{
		isGround = false;
	}
}
