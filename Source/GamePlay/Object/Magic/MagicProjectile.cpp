#include "MagicProjectile.h"
#include "System/Effect/EffectManager.h"


// コンストラクタ
MagicProjectile::MagicProjectile(MagicManager* manager, const std::string& effectPath)
	: MagicBase(manager)
{
	Magic = EffectManager::Instance().GetOrLoadEffect(effectPath);
	magicHandle = -1;
}

// デストラクタ
MagicProjectile::~MagicProjectile()
{
}

// 更新処理
void MagicProjectile::Update(float elapsedTime)
{
	if (!isAlive) return;

	switch (state)
	{
	case ProjectileState::Wait:
		waitTimer -= elapsedTime;
		if (waitTimer <= 0.0f)
		{
			if (Magic != nullptr)
			{
				magicHandle = Magic->Play(position, config.scale);
			}

			state = ProjectileState::Fire;
		}

		break;
	case ProjectileState::Fire:
		if (moveComponent)
		{
			moveComponent->UpdateMove(position, direction, speed, elapsedTime);
		}

		// エフェクトの位置を更新
		if (Magic != nullptr && magicHandle != -1)
		{
			Magic->SetPosition(magicHandle, position);
		}

		// 寿命処理
		lifeTimer -= elapsedTime;
		if (lifeTimer < 0.0f)
		{
			// 寿命が尽きたので、消滅処理を呼び出す
			OnTerminate();
			return;
		}

		break;
	}

	//オブジェクト行列を更新
	UpdateTransform();
}

// 描画処理
void MagicProjectile::Render(const RenderContext& rc)
{
}

// デバッグプリミティブ描画
void MagicProjectile::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	switch (shape)
	{
	case HitBoxShape::Sphere:
		// 球を描画
		renderer->DrawSphere(position, radius, { 1, 0, 1, 1 });

		break;
	case HitBoxShape::Capsule:
		// 中心座標・向き・長さからカプセル描画用の行列を作成
		DirectX::XMVECTOR cPos = DirectX::XMLoadFloat3(&position); // 中心座標
		DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);

		// 方向ベクトルの長さが0なら、デフォルトの前方向 (0, 0, 1) に差し替える
		if (DirectX::XMVector3Equal(dir, DirectX::XMVectorZero()))
		{
			dir = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		}
		else
		{
			dir = DirectX::XMVector3Normalize(dir);
		}

		DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		if (fabsf(direction.y) > 0.999f)
		{
			up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		}

		DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(cPos, dir, up); // ここでエラーが起きてる
		DirectX::XMMATRIX world = DirectX::XMMatrixInverse(nullptr, view);

		// Y軸方向へ伸ばすために90度回転
		world = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90.0f)) * world;

		DirectX::XMFLOAT4X4 capsuleTransform;
		DirectX::XMStoreFloat4x4(&capsuleTransform, world);

		// 向きの合ったカプセルを描画！
		renderer->DrawCapsule(capsuleTransform, radius, height, { 1.0f, 0.0f, 1.0f, 1.0f });

		break;
	}
}

// エフェクトを消す関数
void MagicProjectile::OnTerminate()
{
	if (!GetIsAlive())
		return; // 二度消さない

	isAlive = false;

	if (magicHandle != -1)
	{
		Magic->Stop(magicHandle);
		magicHandle = -1;
	}

	// 自分を削除
	Destroy();
}

// 発射
void MagicProjectile::Launch(const DirectX::XMFLOAT3& dir, const DirectX::XMFLOAT3& pos, float waitTime)
{
	this->direction = dir;
	this->position = pos;
	this->waitTimer = waitTime;
	this->state = ProjectileState::Wait;
}
