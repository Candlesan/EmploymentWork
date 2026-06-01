#include "StraightMagic.h"

// コンストラクタ
StraightMagic::StraightMagic(MagicManager* manager)
	: MagicBase(manager) // 基底クラスのコンストラクタを呼び出す
{
	// おそらくここにパーティクルを呼び出したり
	// 当たり判定が入る

	// ダメージとかのパラメーター初期化
}

// デストラクタ
StraightMagic::~StraightMagic()
{
	// ワンチャンなにも書かない
}

// 更新処理
void StraightMagic::Update(float elapsedTime)
{
	// 移動
	float speed = this->speed * elapsedTime;
	position.x += direction.x * speed;
	position.y += direction.y * speed;
	position.z += direction.z * speed;

	// 寿命処理
	lifeTimer -= elapsedTime;
	if (lifeTimer < 0.0f)
	{
		// 自分を削除
		 Destroy();
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
	const DirectX::XMFLOAT3& position)
{
	this->direction = direction;
	this->position = position;
}
