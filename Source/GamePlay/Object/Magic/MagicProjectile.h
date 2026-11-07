#pragma once
#include "GamePlay/Object/Magic/MagicBase.h"
#include "GamePlay/Object/Magic/IMagicMove.h"
#include "GamePlay/Object/Magic/MagicInfor.h"

#include "System/Effect/Effect.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffect.h"

#include <memory>
#include <string>


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

	float GetSpeed() const { return speed; }
	void SetWaitTimer(float timer) { waitTimer = timer; }
	void SetLifeTimer(float timer) { lifeTimer = timer; }
	void SetSpeed(float s) { speed = s; }

	// ãóó£å∏êäÇÃê›íËÅiégÇÌÇÒíeÇÕåƒÇŒÇÒÇ©Ç¡ÇΩÇÁç°Ç‹Ç≈í ÇËÅj
	void SetFalloff(float start, float end, float minRate)
	{
		useFalloff = true;
		falloffStart = start;
		falloffEnd = end;
		falloffMinRate = minRate;
	}
	float GetDamage() const override;
private:
	float waitTimer = 0.0f; // ë“ã@éûä‘
	float lifeTimer = 3.0f; // éıñΩ
	float speed = 5.0f; // íeë¨
	enum class ProjectileState {Wait, Fire}state = ProjectileState::Wait;

	DirectX::XMFLOAT3 launchPos = {};  // åÇÇ¡ÇΩà íu
	bool  useFalloff = false;
	float falloffStart = 0.0f;   // Ç±Ç±Ç‹Ç≈ÇÕ100%
	float falloffEnd = 0.0f;     // Ç±Ç±Ç≈ç≈í·ÇÃî{ó¶Ç…Ç»ÇÈ
	float falloffMinRate = 1.0f; // ç≈í·ÇÃî{ó¶

	Effect* Magic = nullptr;
	int magicHandle = -1;

	std::unique_ptr<IMagicMove> moveComponent;

	EffectHandle customEffect = InvalidEffect;
	CustomEffectManager& effectManager = CustomEffectManager::Instance();
	std::string effectName = "";
};