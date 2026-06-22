#pragma once
#include "GamePlay/Object/Magic/MagicBase.h"
#include "GamePlay/Object/Magic/MagicManager.h"

#include "System/Effect/Effect.h"

enum class StraightMagicState
{
	Wait,
	Fire
};

// 真っすぐ跳ぶ魔法
class StraightMagic : public MagicBase
{
public:
	StraightMagic(MagicManager* manager, const std::string& effectPath, const std::string& magicCirclePath = "");
	~StraightMagic() override;

	//更新処理
	void Update(float elapsedTime) override;

	//描画処理
	void Render(const RenderContext& rc) override;

	// デバックプリミティブ描画
	void RenderDebugPrimitive(ShapeRenderer* renderer) override;

	//発射
	void Launch(const DirectX::XMFLOAT3& direction, const DirectX::XMFLOAT3& position, float waitTime);

	//消滅するときのリセット処理
	void OnTerminate() override;

private:
	StraightMagicState state = StraightMagicState::Wait;
	float waitTimer = 0.0f; // 発射までの時間
	float radius = 0.2f; // 大きさ
	float speed = 5.0f; // 速度
	float lifeTimer = 3.0f;

	bool isAlive = false; // まだ存在しているか

	// エフェクト
	Effect* Magic = nullptr;
	Effect* Circle = nullptr;
	int magicHandle = -1;
	int circleHandle = -1;
};