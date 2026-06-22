#pragma once
#include "GamePlay//Object/Magic/MagicBase.h"
#include "System/Effect/Effect.h"
#include "System/Graphic/RenderContext.h"

enum class BloodSpearState
{
	Wait, // 魔法陣を出して待機中
	Fire  // プレイヤーに向けて跳んでいる状態
};

class BloodSpearMagic : public MagicBase
{
public:
	BloodSpearMagic(MagicManager* manaher);
	~BloodSpearMagic() override;

	void Update(float elapsedTime) override;
	void Render(const RenderContext& rc) override;
	void RenderDebugPrimitive(ShapeRenderer* renderer) override;
	void OnTerminate() override;

	// 発射準備
	void Launch(const DirectX::XMFLOAT3& position, float waitTime);

private:
	BloodSpearState state = BloodSpearState::Wait; // 待機状態
	float waitTimer = 0.0f; // 発射までの時間
	float speed = 25.0f; // 速さ
	float lifeTimer = 5.0f; // 跳んでから寿命
	float targetDistance = 5.0f; // プレイヤーまでの距離（カプセルの長さ）

	bool isAlive = false; // まだ存在しているか

	// エフェクト関係
	Effect* magicCircle = nullptr;
	Effect* bloodSpear = nullptr;
	int handle = -1;
	bool OnEffect = true;
};