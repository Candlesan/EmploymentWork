#pragma once
#include "GamePlay/Object/Magic/MagicBase.h"
#include "System/Effect/Effect.h"

enum class HommingMagicState
{
	Wait,
	Fire
};

class HommingMagic : public MagicBase
{
public:
	HommingMagic(MagicManager* manager, const std::string& effectPath, const std::string& magicCirclePath = "");
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
				const DirectX::XMFLOAT3& target,
				float waitTime);

	//消滅するときのリセット処理
	void OnTerminate() override;


private:
	HommingMagicState state = HommingMagicState::Wait;
	float waitTimer = 0.0f; // 発射までの時間
	float power = 10; // 出力
	float radius = 0.5; // 大きさ

	DirectX::XMFLOAT3 target = { 0, 0, 0 };
	float    moveSpeed = 10.0f;
	float    turnSpeed = DirectX::XMConvertToRadians(180);
	float    lifeTimer = 3.0f;

	bool isAlive = false; // まだ存在しているか
	bool isHoming = true;

	// エフェクト
	Effect* Magic = nullptr;
	Effect* Circle = nullptr;
	int magicHandle = -1;
	int circleHandle = -1;
};

