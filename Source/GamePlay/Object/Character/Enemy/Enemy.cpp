#include "Enemy.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/ActionDerived.h"
#include "GamePlay/Object/Character/Enemy/BehaviorTree/JudgmentDerived.h"
#include "GamePlay/Object/Character/Player/Player.h"

#include <imgui.h>

// 初期化
void Enemy::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	enemy = std::make_shared<Model>(device, "Data/Model/Enemy/SK_Werewolf.gltf");

	position = { 0, 0, 10 };
	angle = {0, 3, 0};
	scale.x = scale.y = scale.z = 1.2f;
	weight = 100.0f;
	height = 1.0f;
	debugOffset = 0.8;

	health = 30000;
	MaxHealth = 30000.0f;
	maxPoise = 1200.0f;
	currentPoise = 1200.0f;

	invincibleTimer = 0.0f;

	// アニメーション設定
	AnimationStateManager<EnemyAnimationState>::Instance();
	enemy->GetNodePoses(nodePoses);
	enemy->GetNodePoses(oldNodePoses);
	rootMotionNodeName = "Root";
	upperBodyNodeName = "Bip001-Pelvis";
	ChangeAnimationState(EnemyAnimationState::Idle);

	// ビヘイビアツリーの設定
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree();

	// Root
	aiTree->AddNode("", "Root", 0, BehaviorTree::SelectRule::Priority, nullptr, nullptr);

	aiTree->AddNode("Root", "Pursuit", 1, BehaviorTree::SelectRule::Priority, new PursuitJudgment(this), new PursuitAction(this));
	aiTree->AddNode("Root", "Idle", 2, BehaviorTree::SelectRule::Priority, new IdleJudgment(this), new IdleAction(this));

}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	// 現在実行されているノードが無ければ
	if (activeNode == nullptr)
	{
		// 次に実行するノードを推論する。
		activeNode = aiTree->ActiveNodeInference(behaviorData);
	}
	// 現在実行するノードがあれば
	if (activeNode != nullptr)
	{
		// ビヘイビアツリーからノードを実行。
		activeNode = aiTree->Run(activeNode, behaviorData, elapsedTime);
	}

	// ステータス更新
	UpdateStatus(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// 状態遷移更新処理
	UpdateStateTransitions(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

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
	ImGui::Separator();

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

	// 距離判定

	ImGui::End();
}

// デバックプリミティブ描画
void Enemy::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	// 敵の当たり判定
	{
		DirectX::XMFLOAT4X4 capsuleTransform;
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + debugOffset, position.z);
		DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

		renderer->DrawCapsule(capsuleTransform, radius, height, { 1, 1, 0, 1 });
	}

	// 距離の可視化
	{
		// 円の高さ（地面から少し浮かせてチラつきを防止する）
		float circleY = position.y + 0.05f;
		// 円の厚み（極限まで薄くして「円」に見せる）
		float thickness = 0.01f;
		renderer->DrawCylinder(
			{ position.x, circleY, position.z },
			5.0f, thickness, { 1.0f, 0.0f, 0.0f, 1.0f }
		);

		// 中距離 (例: 10.0m) - 黄色
		renderer->DrawCylinder(
			{ position.x, circleY, position.z },
			10, thickness, { 1.0f, 1.0f, 0.0f, 1.0f }
		);

		// 遠距離 (例: 15.0m) - 緑色
		renderer->DrawCylinder(
			{ position.x, circleY, position.z },
			15, thickness, { 0.0f, 1.0f, 0.0f, 1.0f }
		);
	}
}

// プレイヤーとの距離を取得
float Enemy::GetDistanceToPlayer() const
{
	// プレイヤーを取得
	Player& player = Player::Instance();

	// プレイヤーと敵の位置を取得する
	DirectX::XMVECTOR ePos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR pPos = DirectX::XMLoadFloat3(&player.GetPosition());

	// 今の位置とプレイヤーの位置の差を計算する
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(pPos, ePos);
	DirectX::XMVECTOR Length = DirectX::XMVector3Length(Vec);

	// 計算結果を保存する
	float distance = 0.0f;
	DirectX::XMStoreFloat(&distance, Length);

	return distance;
}

// アニメーション更新処理
void Enemy::UpdateStateTransitions(float elapsedTime)
{
	switch (currentState)
	{
	case EnemyAnimationState::Idle:
		if (GetAsyncKeyState('1') & 0x8000) ChangeAnimationState(EnemyAnimationState::Light_Attack_01);
		if (GetAsyncKeyState('2') & 0x8000) ChangeAnimationState(EnemyAnimationState::Skill_DoubleSwings_Root);

		break;
	case EnemyAnimationState::Walk_F:
		break;
	case EnemyAnimationState::Walk_B:
		break;
	case EnemyAnimationState::Walk_L:
		break;
	case EnemyAnimationState::Walk_R:
		break;
	case EnemyAnimationState::Jog_F:
		break;
	case EnemyAnimationState::Dodge_Forkward:
		break;
	case EnemyAnimationState::Dodge_Backward:
		break;
	case EnemyAnimationState::Dodge_Left:
		break;
	case EnemyAnimationState::Dodge_Right:
		break;
	case EnemyAnimationState::Light_Attack_01:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Light_Attack_02:
		break;
	case EnemyAnimationState::Light_Attack_03:
		break;
	case EnemyAnimationState::Heavy_Attack_01:
		break;
	case EnemyAnimationState::Heavy_Attack_02:
		break;
	case EnemyAnimationState::Dodge_FU:
		break;
	case EnemyAnimationState::Grab_Fall:
		break;
	case EnemyAnimationState::Roar:
		break;
	case EnemyAnimationState::Skill_BlockBreaker:
		break;
	case EnemyAnimationState::Skill_DoubleSwings_Root:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);

		break;
	case EnemyAnimationState::Skill_EndlessStabs:
		break;
	case EnemyAnimationState::Skill_QuickStab:
		break;
	case EnemyAnimationState::Skill_HeavyStomp:
		break;
	case EnemyAnimationState::Skill_Leaping:
		break;
	case EnemyAnimationState::Skill_ShoulderBarge_Root:
		break;
	case EnemyAnimationState::Skill_UpperCut:
		break;
	case EnemyAnimationState::Skill_WieldDagger:
		break;
	case EnemyAnimationState::Hit_Front:
		break;
	case EnemyAnimationState::Hit_Light_Left:
		break;
	case EnemyAnimationState::Hit_Light_Right:
		break;
	case EnemyAnimationState::Hit_Launch_Root:
		break;
	case EnemyAnimationState::Hit_Knockdown:
		break;
	case EnemyAnimationState::Death_A:
		break;
	case EnemyAnimationState::Death_B:
		break;
	default:
		break;
	}
}

void Enemy::OnDown()
{
	Ondown = true;
}
