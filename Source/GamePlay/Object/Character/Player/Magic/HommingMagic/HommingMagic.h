#pragma once
#include "GamePlay/Object/Character/Player/Magic/MagicBase.h"
#include "System/Effect/Effect.h"


class HommingMagic : public MagicBase
{
public:
	HommingMagic(MagicManager* manager);
	~HommingMagic() override;

	//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render(const RenderContext& rc) override;

	// デバックプリミティブ描画
	void RenderDebugPrimitive(ShapeRenderer* renderer) override;

	//発射
	void Launch(const DirectX::XMFLOAT3& direction,
				const DirectX::XMFLOAT3& position,
				const DirectX::XMFLOAT3& target);

	//消滅するときのリセット処理
	void OnTerminate() override;


	float GetDamage() const override { return damage; }
private:

	float damage = 2.0f; // ダメージ量
	float power = 10; // 出力
	float radius = 1; // 大きさ
	float Magic_Consumption = 1; // 魔力消費量

	DirectX::XMFLOAT3 target = { 0, 0, 0 };
	float    moveSpeed = 10.0f;
	float    turnSpeed = DirectX::XMConvertToRadians(180);
	float    lifeTimer = 3.0f;

	bool isAlive = false; // まだ存在しているか
	bool isHoming = true;

	// エフェクト
	std::unique_ptr<Effect> testEffect;
	int handle = -1;
	bool OnEffect = true;
};

