#include "Gameplay/Object/Character/Character.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/Stage/Stage.h"

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
		else if (status.isDown || isGround) // ダウン中は急速に回復させる
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
		if (status.currentHeat >= status.maxHeat)
		{
			status.currentHeat = status.maxHeat;
			status.isBurstTrigger = true;
			status.isBuring = true;
		}
	}
	else
	{
		if (status.currentHeat > 0.0f)
		{
			status.isBurstTrigger = false;

			// ヒート値があったら下げていく
			if (status.isBuring)
			{
				status.currentHeat -= (status.heatRecoverSpeed * elapsedTime) * BurstCoolDownScale;
			}
			else
			{
				status.currentHeat -= status.heatRecoverSpeed * elapsedTime;

			}

			// ヒート値が0よりも低かったら
			if (status.currentHeat <= 0.0f)
			{
				status.currentHeat = 0.0f;
				status.isBuring = false;
			}
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

		if (status.isBreak)
		{
			status.currentDefenseDurability += status.defenseRecoverSpeed * elapsedTime * 3.0f;
		}

		// 現在の値が最大値を超えていたら
		if (status.currentDefenseDurability > status.maxDefenseDurability)
		{
			status.currentDefenseDurability = status.maxDefenseDurability;
			status.isBreak = false; // 最大になったらfalseにする
		}
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

// ヒート値を回復する
void Character::RecoverHeat(float amount)
{
	status.currentHeat -= amount;

	// 0を下回らないようにする
	if (status.currentHeat <= 0.0f)
	{
		status.currentHeat = 0.0f;
		status.isBurstTrigger = false;
		status.isBuring = false;
	}
}

// ガード耐久値減少
void Character::OnGuardHit(float amount)
{
	status.currentDefenseDurability -= amount;

	if (status.currentDefenseDurability <= 0.0f)
	{
		status.currentDefenseDurability = 0.0f;
		status.isBreak = true;
	}
}

//水平速力更新処理
void Character::UpdateHorizontalVelocity(float elapsedTime)
{
	//XZ平面の速力を減速
	float length = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
	if (length > 0.0f)
	{
		// 空中にいる時は摩擦をほぼ0にする
		float currentFriction = isGround ? (this->friction * elapsedTime) : (this->friction * 0.5f * elapsedTime);

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
			float currentAcceleration = isGround ? (this->acceleration * elapsedTime) : (this->acceleration * 1.0f * elapsedTime);

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

//水平移動更新処理
void Character::UpdateHorizontalMove(float elapsedTime)
{
	if (stage)
	{
		MoveAndSlide({ velocity.x * elapsedTime, 0.0f, velocity.z * elapsedTime }, false);
	}
	else
	{
		//移動処理
		position.x += velocity.x * elapsedTime;
		position.z += velocity.z * elapsedTime;
	}
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
	if (stage)
	{
		float my = velocity.y * elapsedTime;
		if (my <= 0.0f) // 落下/接地
		{
			if (MoveAndSlide({ 0.0f, my, 0.0f }, true))
			{
				// 地面に着いたとき
				velocity.y = 0.0f;
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
		else // 上昇中
		{
			position.y += my;
			isGround = false;
		}
	}
	else
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
}

bool Character::MoveAndSlide(const DirectX::XMFLOAT3& move, bool vertical)
{
	DirectX::XMVECTOR Position = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR Move = DirectX::XMLoadFloat3(&move);
	DirectX::XMVECTOR CenterOffset = DirectX::XMVectorSet(0, collisionRadius + stepOffset, 0, 0);

	// スフィアキャストによる移動＆滑り処理
	{
		for (int i = 0; i < 3; ++i)
		{
			//キャスト量を算出
			//GetXだけのスカラー値(大きさ)だけでは実際にどれだけ移動するか分からないからLength関数で長さを計算し実際の移動距離とする
			float distance = DirectX::XMVectorGetX(DirectX::XMVector3Length(Move));
			if (distance < 0.0001f) break;

			//壁より少し手前で止まって欲しいので少しだけキャスト量を増やす
			distance += 0.01;

			//縦移動はステップオフセット分もキャスト量を増やす
			if (vertical)
			{
				distance += stepOffset;
			}

			DirectX::XMVECTOR Origin = DirectX::XMVectorAdd(Position, CenterOffset); //開始位置
			DirectX::XMVECTOR Direction = DirectX::XMVector3Normalize(Move); //方向
			DirectX::XMFLOAT3 origin, direction;
			DirectX::XMStoreFloat3(&origin, Origin);
			DirectX::XMStoreFloat3(&direction, Direction);
			DirectX::XMFLOAT3 hitPosition, hitNormal;
			if (Collision::SphereCast(origin, direction, collisionRadius, distance, stage->GetTriangles(), stage->GetAreas(), hitPosition, hitNormal, true))
			{
				//キャスト量を増やした分だけ減らす
				DirectX::XMVECTOR HitPos = DirectX::XMLoadFloat3(&hitPosition);
				float actualDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(
					DirectX::XMVectorSubtract(HitPos, Origin)));
				float moveDistance = actualDistance - 0.01f;

				if (vertical)
				{
					moveDistance -= stepOffset;
				}

				//法線ベクトル
				DirectX::XMVECTOR HitNormal = DirectX::XMVector3Normalize(
					DirectX::XMLoadFloat3(&hitNormal));

				//スロープチェック
				if (vertical)
				{
					//スロープ角度算出
					DirectX::XMVECTOR Up = DirectX::XMVectorSet(0, 1, 0, 1);
					float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(HitNormal, Up));
					float angle = acos(dot);

					//制限角度以内なら滑る処理はスキップ
					if (angle < slopeLimit)
					{
						// 移動して保存する
						Position = DirectX::XMVectorAdd(Position, DirectX::XMVectorScale(Direction, moveDistance));
						DirectX::XMStoreFloat3(&position, Position);
						return true;
					}

					// 急な面では持ち上げないようにする
					if (moveDistance < 0.0f) moveDistance = 0.0f;
				}
				else
				{
					// 水平移動では法線のYを消して、壁ずりで上に滑らないようにする
					HitNormal = DirectX::XMVectorSetY(HitNormal, 0.0f);
					if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(HitNormal)) < 0.0001f) break; // HitNormalの長さが小さかったらスキップする
					HitNormal = DirectX::XMVector3Normalize(HitNormal); // 正規化して保存する
				}

				//移動量を計算
				DirectX::XMVECTOR Vec = DirectX::XMVectorScale(Direction, moveDistance);

				//移動
				Position = DirectX::XMVectorAdd(Position, Vec);

				//滑り処理
				{
					//移動した量を減らす
					// ※移動ベクトルを求めている(方向×距離)
					DirectX::XMVECTOR usedMove = DirectX::XMVectorScale(Direction, moveDistance);
					Move = DirectX::XMVectorSubtract(Move, usedMove);


					float remainingLen = DirectX::XMVectorGetX(DirectX::XMVector3Length(Move));
					if (remainingLen < 0.0001f) break;

					//次回移動した壁に沿うように移動ベクトルを算出
					DirectX::XMVECTOR moveDir = DirectX::XMVector3Normalize(Move);
					DirectX::XMVECTOR dot = DirectX::XMVector3Dot(moveDir, HitNormal);
					DirectX::XMVECTOR proj = DirectX::XMVectorScale(HitNormal, DirectX::XMVectorGetX(dot));
					DirectX::XMVECTOR slideVec = DirectX::XMVectorSubtract(moveDir, proj);

					Move = DirectX::XMVectorScale(slideVec, remainingLen);
				}
			}
			else
			{
				//当たらなかったので普通に移動
				Position = DirectX::XMVectorAdd(Position, Move);
				break;
			}
		}
	}

	DirectX::XMStoreFloat3(&position, Position);
	return false;
}
