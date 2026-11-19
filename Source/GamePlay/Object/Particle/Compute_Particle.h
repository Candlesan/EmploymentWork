#pragma once
#include <memory>
#include <string>
#include "GamePlay/Object/Particle/compute_particle_system.h"
#include "System/Graphic/RenderContext.h"


class Compute_Particle
{
public:
	Compute_Particle(ID3D11Device* device, int maxParticle = 128);
	~Compute_Particle() {};

	void Initialize(ID3D11Device* device);
	void Finalize();
	void Update(ID3D11DeviceContext* dc, DirectX::XMFLOAT3 parent_delta, float elapsedTime);
	void Render(const RenderContext& rc);
	void DrawGUI();

	// 外部からエフェクトを発生させる窓口
	void Emit(const compute_particle_system::emit_particle_data& data) {
		if (system) system->emit(data);
	}

	void Clear(ID3D11DeviceContext* dc);

	// ゲッターセッター
	bool SetTexture(const std::string& texturePath, DirectX::XMUINT2 splitCount = { 1, 1 });

	void SetParent(const DirectX::XMFLOAT4X4& world, bool local)
	{
		if (system) system->set_parent(world, local);
	}
	
	void SetBlendMode(int mode) { blendMode = mode; }

	void SetDissolve(int mode, float value)
	{
		if (system)system->set_dissolve(mode, value);
	}

	void SetTrail(bool enable, float width, float interval, int align)
	{
		if (system) system->set_trail(enable, width, interval, align);
	}

	void SetForces(const DirectX::XMFLOAT4& vortex,
		const DirectX::XMFLOAT4& attract,
		const DirectX::XMFLOAT4& noise)
	{
		if (system) system->set_forces(vortex, attract, noise);
	}

private:
	ID3D11Device* device = nullptr; // テクスチャ再ロード用に保持

	// シーン定数バッファ
	struct CBScene
	{
		DirectX::XMFLOAT4X4 viewProjection;
		DirectX::XMFLOAT4 lightDirection; // 使わなくても必要！
		DirectX::XMFLOAT4 lightColor;     // 使わなくても必要！
		DirectX::XMFLOAT4 cameraPosition; // 使わなくても必要！
		DirectX::XMFLOAT4X4 inverseView;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;

	std::unique_ptr<compute_particle_system> system;
	int blendMode = 0; // 0:半透明 1:加算
};