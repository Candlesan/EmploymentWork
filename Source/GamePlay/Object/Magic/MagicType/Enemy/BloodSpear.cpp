#include "BloodSpear.h"
#include "System/Effect/EffectManager.h"
#include "GamePlay/Object/Character/Player/Player.h"


// コンストラクタ
BloodSpearMagic::BloodSpearMagic(MagicManager* manager)
	: MagicBase(manager)
{
	// 魔法陣
	magicCircle = EffectManager::Instance().GetOrLoadEffect("Data/Effect/magic_circle/magicCircle_blood.efk");
	bloodSpear = EffectManager::Instance().GetOrLoadEffect("Data/Effect/BloodSpear/spear.efk");
	magicCircleHandle = -1;
	spearHandle = -1;
	OnEffect = true;
	this->damage = 100.0f;
}

BloodSpearMagic::~BloodSpearMagic()
{
}

void BloodSpearMagic::Update(float elapsedTime)
{
	if (isAlive) return;

	switch (state)
	{
	case BloodSpearState::Wait: // 待機中
	{
		// 待機時間を減らす
		waitTimer -= elapsedTime;

		Player& player = Player::Instance();
		DirectX::XMVECTOR Ppos = DirectX::XMLoadFloat3(&player.GetPosition());
		DirectX::XMVECTOR Epos = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Ppos, Epos);

		DirectX::XMStoreFloat(&height, DirectX::XMVector3Length(Vec));

		Vec = DirectX::XMVector3Normalize(Vec);
		DirectX::XMStoreFloat3(&direction, Vec);

		// 待機時間が切れたら発射する方向を決める
		if (waitTimer < 0.0f)
		{
			height = 10.0f;

			state = BloodSpearState::Fire; // ステートを変える
		}

		break;
	}
	case BloodSpearState::Fire: // 発射
	{
		// directionに従って弾を進める
		float speed = this->speed * elapsedTime;
		position.x += direction.x * speed;
		position.y += direction.y * speed;
		position.z += direction.z * speed;

		if (OnEffect)
		{
			spearHandle = bloodSpear->Play(position, 1.0f);

			// 向きを計算する
			float angleY = atan2f(direction.x, direction.z);
			float horizontalLength = sqrtf(direction.x * direction.x + direction.z * direction.z);
			float angleX = -atan2f(direction.y, horizontalLength);
			angleY += DirectX::XMConvertToRadians(180.0f);
			bloodSpear->SetAngle(spearHandle, DirectX::XMFLOAT3(angleX, angleY, 0.0f));

			OnEffect = false;
		}

		bloodSpear->SetPosition(spearHandle, position);

		// 寿命を減らす
		lifeTimer -= elapsedTime;
		if (lifeTimer < 0)
		{
			OnTerminate();
			return;
		}
		break;
	}
	}

	// オブジェクト行列更新
	UpdateTransform();
}

// 描画処理
void BloodSpearMagic::Render(const RenderContext& rc)
{
}

// デバックプリミティブ描画
void BloodSpearMagic::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	radius = 2.0f;

	// カプセルの「中心位置」を計算する
	// ※カプセルは中心から描画されることが多いので、自分の位置とプレイヤーの中間地点を求めます
	DirectX::XMFLOAT3 centerPos;
	centerPos.x = position.x + (direction.x * height * 0.5f);
	centerPos.y = position.y + (direction.y * height * 0.5f);
	centerPos.z = position.z + (direction.z * height * 0.5f);

	// DirectXMath を使って、向き（回転）と位置を持った 4x4 行列を作る
	DirectX::XMVECTOR cPos = DirectX::XMLoadFloat3(&centerPos);
	DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 基本の「上」方向

	// 安全対策（真上や真下を向いた時に計算がおかしくなるのを防ぐ）
	if (fabsf(direction.y) > 0.999f) {
		up = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}

	// カメラの「View行列」を作る関数を逆利用する！
	DirectX::XMMATRIX view = DirectX::XMMatrixLookToLH(cPos, dir, up);
	DirectX::XMMATRIX world = DirectX::XMMatrixInverse(nullptr, view);

	// ※ もし描画したカプセルが「縦に立ったまま平行移動」してしまう場合は、
	// カプセル自体のモデルがY軸基準で作られているため、以下の1行のコメントアウトを外して90度倒してください！
	 world = DirectX::XMMatrixRotationX(DirectX::XMConvertToRadians(90.0f)) * world;

	// 3. 計算した行列を XMFLOAT4X4 に変換して描画関数に渡す
	DirectX::XMFLOAT4X4 capsuleTransform;
	DirectX::XMStoreFloat4x4(&capsuleTransform, world);

	// 赤色 { 1.0f, 0.0f, 0.0f, 1.0f } で描画！
	renderer->DrawCapsule(capsuleTransform, radius, height, { 1.0f, 0.0f, 0.0f, 1.0f });
}

// 寿命が消えたらエフェクトを消す処理
void BloodSpearMagic::OnTerminate()
{
	isAlive = true;

	// 魔法陣を止める
	if (magicCircleHandle != -1 && magicCircle != nullptr)
	{
		magicCircle->Stop(magicCircleHandle);
		magicCircleHandle = -1;
	}

	// 槍を止める
	if (spearHandle != -1 && bloodSpear != nullptr)
	{
		bloodSpear->Stop(spearHandle);
		spearHandle = -1;
	}

	// 自分を削除
	Destroy();
}

// 発射処理
void BloodSpearMagic::Launch(const DirectX::XMFLOAT3& position, float waitTime)
{
	this->position = position;
	waitTimer = waitTime;

	if (magicCircle != nullptr) {
		magicCircleHandle = magicCircle->Play(this->position, 1.0f); // 1.0fはサイズ
		magicCircle->SetAngle(magicCircleHandle, direction);
	}
}
