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
	void SetGrow(float range, float duration)
	{
		useGrow = true;
		maxRange = range;
		growDuration = duration;
		growTimer = 0.0f;
	}

	// ‹——£Œ¸Š‚Ìİ’èig‚í‚ñ’e‚ÍŒÄ‚Î‚ñ‚©‚Á‚½‚ç¡‚Ü‚Å’Ê‚èj
	void SetFalloff(float start, float end, float minRate)
	{
		useFalloff = true;
		falloffStart = start;
		falloffEnd = end;
		falloffMinRate = minRate;
	}
	float GetDamage() const override;
private:
	float waitTimer = 0.0f; // ‘Ò‹@ŠÔ
	float lifeTimer = 3.0f; // õ–½
	float speed = 5.0f; // ’e‘¬
	enum class ProjectileState {Wait, Fire}state = ProjectileState::Wait;

	DirectX::XMFLOAT3 launchPos = {};  // Œ‚‚Á‚½ˆÊ’u
	bool  useFalloff = false;
	float falloffStart = 0.0f;   // ‚±‚±‚Ü‚Å‚Í100%
	float falloffEnd = 0.0f;     // ‚±‚±‚ÅÅ’á‚Ì”{—¦‚É‚È‚é
	float falloffMinRate = 1.0f; // Å’á‚Ì”{—¦

	float growTimer = 0.0f;    // Œo‰ßŠÔ
	float growDuration = 1.0f; // ‚Ç‚ê‚¾‚¯ŠÔ‚ğ‚©‚¯‚é‚©
	float maxRange = 1.0f; // Ë’ö
	bool useGrow = false; // L‚Î‚·‹…‚©

	Effect* Magic = nullptr;
	int magicHandle = -1;

	std::unique_ptr<IMagicMove> moveComponent;

	EffectHandle customEffect = InvalidEffect;
	CustomEffectManager& effectManager = CustomEffectManager::Instance();
	std::string effectName = "";
};