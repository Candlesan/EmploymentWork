#include "Player.h"
#include "System/Graphic/Graphics.h"
#include <imgui.h>

// 初期化
void Player::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	player = std::make_shared<Model>(device, "Data/Model/Player/Map_Robot3.gltf");
}

// 更新処理
void Player::Update(float elapsedTime)
{
	// モデル更新処理
	UpdateTransform();
	player->UpdateTransform(transform);
}

// 描画処理
void Player::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::Lambert, player); 
}

// GUI描画
void Player::DrawGUI()
{
	ImGui::Begin("Player");

	ImGui::End();
}