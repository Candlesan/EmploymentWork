#pragma once
#include "GamePlay/Object/Magic/MagicBase.h"
#include "GamePlay/Object/Magic/IMagicMove.h"
#include "GamePlay/Object/Magic/MagicInfor.h"

#include "System/Effect/Effect.h"

#include <memory>
#include <string>

// ëOï˚êÈåæ
class CustomEffect;

class MagicProjectile : public MagicBase
{
public:
	MagicProjectile(MagicManager* manager, const std::string& effectPath = "");
	~MagicProjectile() override;

	void Update(float elapsedTime) override;
	void Render(const RenderContext& rc)override;
	void RenderDebugPrimitive(ShapeRenderer* renderer)override;
	void OnTerminate()override; 


	void SetMoveComponent(std::unique_ptr<IMagicMove> move) { moveComponent = std::move(move); }

	void Launch(const DirectX::XMFLOAT3& dir, const DirectX::XMFLOAT3& pos, float waitTime);

	float GetWaitTimer() const { return waitTimer; }
	float GetLifeTimer() const { return lifeTimer; }
	float GetSpeed() const { return speed; }
	void SetWaitTimer(float timer) { waitTimer = timer; }
	void SetLifeTimer(float timer) { lifeTimer = timer; }
	void SetSpeed(float s) { speed = s; }


private:
	float waitTimer = 0.0f;
	float lifeTimer = 3.0f;
	float speed = 5.0f;
	enum class ProjectileState {Wait, Fire}state = ProjectileState::Wait;

	Effect* Magic = nullptr;
	int magicHandle = -1;

	std::unique_ptr<IMagicMove> moveComponent;

	MagicInfo config;

	CustomEffect* customEffect = nullptr;
	std::string effectName = "";
};