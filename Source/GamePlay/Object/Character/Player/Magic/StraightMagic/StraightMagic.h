#pragma once
#include "GamePlay/Object/Character/Player/Magic/MagicBase.h"
#include "GamePlay/Object/Character/Player/Magic/MagicManager.h"

#include "System/Effect/Effect.h"

// 真っすぐ跳ぶ魔法
class StraightMagic : public MagicBase
{
public:
	StraightMagic(MagicManager* manager);
	~StraightMagic() override;

	//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render(const RenderContext& rc) override;

	// デバックプリミティブ描画
	void RenderDebugPrimitive(ShapeRenderer* renderer) override;

	//発射
	void Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position);

	//消滅するときのリセット処理
	void OnTerminate() override;

	// ダメージ取得
	float GetDamage() const override { return damage; }
private:
	float damage = 1.0f; // ダメージ量
	float power = 10; // 出力
	float radius = 1; // 大きさ
	float Magic_Consumption = 1; // 魔力消費量
	float speed = 5; // 速度
	float lifeTimer = 3.0f;

	bool isAlive = false; // まだ存在しているか

	// エフェクト
	std::unique_ptr<Effect> testEffect;
	int handle = -1;
	bool OnEffect = true;
};