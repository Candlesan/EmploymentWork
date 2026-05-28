#pragma once
#include <memory>
#include "GamePlay/Object/Particle/compute_particle_system.h"
#include "System/Graphic/RenderContext.h"


class Compute_Particle
{
public:
	Compute_Particle(ID3D11Device* device);
	~Compute_Particle() {};

	void Initialize(ID3D11Device* device);
	void Finalize();
	void Update(ID3D11DeviceContext* dc, float elapsedTime);
	void Render(const RenderContext& rc);
	void DrawGUI();

	// 外部からエフェクトを発生させる窓口
	void Emit(const compute_particle_system::emit_particle_data& data) {
		if (system) system->emit(data);
	}
private:
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
};