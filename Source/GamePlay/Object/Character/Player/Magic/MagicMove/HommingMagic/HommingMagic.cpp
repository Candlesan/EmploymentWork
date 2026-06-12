#include "HommingMagic.h"


// コンストラクタ
HommingMagic::HommingMagic(MagicManager* manager)
	:MagicBase(manager)
{
	// エフェクト初期化
	testEffect = std::make_unique<Effect>("Data/Effect/Enemy_Bullet.efk");
	handle = -1;

	OnEffect = true;
}

// デストラクタ
HommingMagic::~HommingMagic()
{
	// ワンチャンなにも書かない
}

// 更新処理
void HommingMagic::Update(float elapsedTime)
{
	if (isAlive) return;

	// 寿命処理
	lifeTimer -= elapsedTime;
	if (lifeTimer < 0.0f)
	{
		// 寿命が尽きたので、消滅処理を呼び出す
		OnTerminate();

		isHoming = true;
		return;
	}

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

	if(isHoming)
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
				//回転軸を算出
				DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(Direction, Vec);
				Axis = DirectX::XMVector3Normalize(Axis);

				//回転軸と回転量から回転行列を算出
				DirectX::XMMATRIX Rotation = DirectX::XMMatrixRotationAxis(Axis, rot);

				//現在の行列を回転させる
				DirectX::XMMATRIX Transform = DirectX::XMLoadFloat4x4(&transform);
				Transform = DirectX::XMMatrixMultiply(Transform, Rotation);

				//回転後の前方方向を取り出し、単位ベクトル化する
				Direction = DirectX::XMVector3Normalize(Transform.r[2]);
				DirectX::XMStoreFloat3(&direction, Direction);
			}

		}
	}


	// 移動
	float speed = this->moveSpeed * elapsedTime;
	position.x += direction.x * speed;
	position.y += direction.y * speed;
	position.z += direction.z * speed;

	if (OnEffect)
	{
		handle = testEffect->Play(position, 0.5);
		OnEffect = false;
	}

	testEffect->SetPosition(handle, position);

	//オブジェクト行列を更新
	UpdateTransform();
}

// 描画処理
void HommingMagic::Render(const RenderContext& rc)
{
}

// デバックプリミティブ描画
void HommingMagic::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	// 球を描画
	renderer->DrawSphere(position, radius, { 1, 0, 1, 1 });
}

// 発射
void HommingMagic::Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target)
{
	this->direction = direction;
	this->position = position;
	this->target = target;

	UpdateTransform();
}

//消滅するときのリセット処理
void HommingMagic::OnTerminate()
{
	isAlive = true;
	if (handle != -1)
	{
		testEffect->Stop(handle);
		handle = -1;
	}

	// 自分を削除
	Destroy();
}
