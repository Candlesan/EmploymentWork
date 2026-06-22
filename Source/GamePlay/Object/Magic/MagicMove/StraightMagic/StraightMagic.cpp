#include "StraightMagic.h"
#include "System/Effect/EffectManager.h"

// コンストラクタ
StraightMagic::StraightMagic(MagicManager* manager, const std::string& effectPath, const std::string& magicCirclePath)
	: MagicBase(manager) // 基底クラスのコンストラクタを呼び出す
{
	// 魔法のエフェクト初期化
	Magic = EffectManager::Instance().GetOrLoadEffect(effectPath);
	magicHandle = -1;

	// 魔法陣のエフェクト初期化
	if (!magicCirclePath.empty())
	{
		Circle = EffectManager::Instance().GetOrLoadEffect(magicCirclePath);
		circleHandle = -1;
	}
}

// デストラクタ
StraightMagic::~StraightMagic()
{
	// ワンチャンなにも書かない
}

// 更新処理
void StraightMagic::Update(float elapsedTime)
{
	if (isAlive) return;

	switch (state)
	{
	case StraightMagicState::Wait:
		waitTimer -= elapsedTime;
		if (waitTimer <= 0.0f)
		{
			if (Magic != nullptr)
			{
				magicHandle = Magic->Play(position, 0.2f);
				state = StraightMagicState::Fire;
			}
		}
		break;
	case StraightMagicState::Fire:
		// 移動
		float speed = this->speed * elapsedTime;
		position.x += direction.x * speed;
		position.y += direction.y * speed;
		position.z += direction.z * speed;

		Magic->SetPosition(magicHandle, position);

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

	// オブジェクト行列更新
	UpdateTransform();
}

// 描画処理
void StraightMagic::Render(const RenderContext& rc)
{
	// パーティクルをここで描画する
}

// デバックプリミティブ描画
void StraightMagic::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	// 球を描画
	renderer->DrawSphere(position, radius, { 0, 0, 1, 1 });
}

// 発射処理
void StraightMagic::Launch(const DirectX::XMFLOAT3& direction,
	const DirectX::XMFLOAT3& position, float waitTime)
{
	this->direction = direction;
	this->position = position;
	this->waitTimer = waitTime;
	this->state = StraightMagicState::Wait;

	if (Circle != nullptr) {
		circleHandle = Circle->Play(this->position, 1.0f); // 1.0fはサイズ
	}
}

//消滅するときのリセット処理
void StraightMagic::OnTerminate()
{
	isAlive = true;

	if (magicHandle != -1)
	{
		Magic->Stop(magicHandle);
		magicHandle = -1;
	}

	if (circleHandle != -1)
	{
		Circle->Stop(circleHandle);
		circleHandle = -1;
	}

	// 自分を削除
	Destroy();
}
