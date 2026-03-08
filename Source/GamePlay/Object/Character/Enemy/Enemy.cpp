#include "Enemy.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"

#include <imgui.h>

// 初期化
void Enemy::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	enemy = std::make_shared<Model>(device, "Data/Model/Enemy/Map_Robot3.gltf");

	position = { 0, 0, 10 };
	angle = {0, 3, 0};
	weight = 100.0f;
	height = 1.0f;
	debugOffset = 0.8;

	health = 30000;
	MaxHealth = 30000.0f;
	maxPoise = 1200.0f;
	currentPoise = 1200.0f;

	invincibleTimer = 0.0f;

	// アニメーション設定
	AnimationStateManager<PlayerAnimationState>::Instance();
	enemy->GetNodePoses(nodePoses);
	enemy->GetNodePoses(oldNodePoses);
	ChangeAnimationState(PlayerAnimationState::Idle);
}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	// アニメーション更新処理
	UpdateAnimations(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// ステータス更新
	UpdateStatus(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	enemy->UpdateTransform(transform);
}

// 描画処理
void Enemy::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, enemy);
}

// GUI描画
void Enemy::DrawGUI()
{
	ImGui::Begin("Enemy");
	AttackResult res;

	if (ImGui::Button(u8"HP前回"))
	{
		health = MaxHealth;
	}
	ImGui::Text("Health: %f.0", health);
	ImGui::Text("Damage: %f.0", lastDamage);
	ImGui::Text("InvincibleTimer: %f.0", invincibleTimer);
	ImGui::Separator();
	ImGui::Text("currentPoise: %f.0", currentPoise);
	ImGui::Text("poiseFullResetTimer: %f.0", poiseFullResetTimer);
	ImGui::Text("poiseRecoveryDelay: %f.0", poiseRecoveryDelay);

	// トランスフォーム情報
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat3("Position", &position.x);

		DirectX::XMFLOAT3 a;
		a.x = DirectX::XMConvertToDegrees(angle.x);
		a.y = DirectX::XMConvertToDegrees(angle.y);
		a.z = DirectX::XMConvertToDegrees(angle.z);
		ImGui::DragFloat3("Angle", &angle.x);
		// 表示用に度数法に変換した後、再度ラジアンで戻す処理
		angle.x = DirectX::XMConvertToRadians(a.x);
		angle.y = DirectX::XMConvertToRadians(a.y);
		angle.z = DirectX::XMConvertToRadians(a.z);

		ImGui::DragFloat3("Scale", &scale.x);
	}

	// 当たり判定情報
	if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Radius:", &radius, 0.1f); // 当たり判定の半径
		ImGui::DragFloat("Height:", &height, 0.1f); // 当たり判定の高さ
		ImGui::DragFloat("Collision Transform Offset:", &debugOffset, 0.1f);
	}

	ImGui::End();
}

// デバックプリミティブ描画
void Enemy::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	DirectX::XMFLOAT4X4 capsuleTransform;
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + debugOffset, position.z);
	DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

	renderer->DrawCapsule(capsuleTransform, radius, height, { 1, 1, 0, 1 });
}

// アニメーション更新処理
void Enemy::UpdateAnimations(float elapsedTime)
{
	switch (currentState)
	{
	case PlayerAnimationState::Idle:
		if (Ondown) ChangeAnimationState(PlayerAnimationState::Guard_Hit_03);

	case PlayerAnimationState::Guard_Hit_03:

		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle);

		break;
	}
}

void Enemy::OnDown()
{
	Ondown = true;
}
