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
	if (isAlive) return;

	switch (state)
	{
	case ProjectileState::Wait:
		waitTimer -= elapsedTime;
		if (waitTimer <= 0.0f)
		{
			if (Magic != nullptr)
			{
				magicHandle = Magic->Play(position, 0.2f);
				state = ProjectileState::Fire;
			}
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
	// 球を描画
	renderer->DrawSphere(position, radius, { 1, 0, 1, 1 });
}

// エフェクトを消す関数
void MagicProjectile::OnTerminate()
{
	isAlive = true;

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
