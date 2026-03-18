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

	weapon[0].model = std::make_shared<Model>(device, "Data/Model/Weapon/Enemy/SK_Werewolf_Type_C_Weapon.gltf");
	weapon[1].model = std::make_shared<Model>(device, "Data/Model/Weapon/Enemy/SK_Werewolf_Type_C_Weapon.gltf");

	// 敵モデルの設定
	position = { 0, 0, 10 };
	angle = {0, 3, 0};
	scale.x = scale.y = scale.z = 1.2f;
	weight = 100.0f;
	radius = 1.0f;
	height = 1.6f;
	debugOffset = 0.8;

	// 武器モデルの設定
	weapon[0].position = { -0.13, 0.01, -0.05 };
	weapon[0].angle = { 0.24, 4.55, -1.90 };
	weapon[0].weaponHitOffset = { -0.83, -0.13, 0.06 };
	weapon[0].weaponAngleOffset = { 0.54, 8.0, 0.43 };
	weapon[0].weaponRadius = 0.59f;
	weapon[0].weaponHeight = 1.7f;

	weapon[1].position = { -0.16, 0.01, -0.03 };
	weapon[1].angle = { 0.04, 4.23, 1.78 };
	weapon[1].weaponHitOffset = { -0.4, 0.02, 0.1 };
	weapon[1].weaponAngleOffset = { 0.0, 2.12, -0.23 };
	weapon[1].weaponRadius = 0.59f;
	weapon[1].weaponHeight = 1.7f;

	// ステータスの設定
	health = 30000;
	MaxHealth = 30000.0f;
	maxPoise = 1200.0f;
	baseAttackPower = 1400.0f;
	currentPoise = 1200.0f;

	invincibleTimer = 0.0f;

	// アニメーション設定
	AnimationStateManager<EnemyAnimationState>::Instance();
	enemy->GetNodePoses(nodePoses);
	enemy->GetNodePoses(oldNodePoses);
	rootMotionNodeName = "Root";
	upperBodyNodeName = "Bip001-Pelvis";
	ChangeAnimationState(EnemyAnimationState::Idle);

	// Jsonファイルの初期化
	InitializeAttackData();

	// ビヘイビアツリーの設定
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree();

	// Root
	aiTree->AddNode("", "Root", 0, BehaviorTree::SelectRule::Priority, nullptr, nullptr);

	aiTree->AddNode("Root", "Pursuit", 1, BehaviorTree::SelectRule::Priority, new PursuitJudgment(this), new PursuitAction(this));
	aiTree->AddNode("Root", "Idle", 2, BehaviorTree::SelectRule::Priority, new IdleJudgment(this), new IdleAction(this));

}

// 攻撃とかの情報を初期化(Jsonファイルの初期化)
void Enemy::InitializeAttackData()
{
	// シーケンサーの初期化
	animSequence.SetModel(enemy);

	// Jsonがあれば読み込む、無ければデフォルト値を設定
	std::ifstream file("Data/Json/Enemy/AttackData/AttackSequence.json");
	if (file.is_open())
	{
		file.close();
		animSequence.Load("Data/Json/Enemy/AttackData/AttackSequence.json");
	}
	else
	{
		animSequence.attackData[EnemyAnimationState::Light_Attack_01] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[EnemyAnimationState::Light_Attack_02] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[EnemyAnimationState::Light_Attack_03] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};

		animSequence.attackData[EnemyAnimationState::Heavy_Attack_01] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[EnemyAnimationState::Heavy_Attack_02] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};

		animSequence.attackData[EnemyAnimationState::Skill_BlockBreaker] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[EnemyAnimationState::Skill_DoubleSwings_Root] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[EnemyAnimationState::Skill_EndlessStabs] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::LeftHand },
		{ 40, 90, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定3", 0xFF0000FF, TrackType::HitBox, HandType::LeftHand },
		{ 40, 90, u8"当たり判定4", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定5", 0xFF0000FF, TrackType::HitBox, HandType::LeftHand },
		};
		animSequence.attackData[EnemyAnimationState::Skill_HeavyStomp] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::Body },
		};
		animSequence.attackData[EnemyAnimationState::Skill_Leaping] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		};
		animSequence.attackData[EnemyAnimationState::Skill_QuickStab] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定3", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		};
		animSequence.attackData[EnemyAnimationState::Skill_ShoulderBarge_Root] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::Body },
		};
		animSequence.attackData[EnemyAnimationState::Skill_UpperCut] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		};
		animSequence.attackData[EnemyAnimationState::Skill_WieldDagger] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定3", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定4", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		{ 40, 90, u8"当たり判定5", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		};

	}
}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	// 現在実行されているノードが無ければ
	//if (activeNode == nullptr)
	//{
	//	 次に実行するノードを推論する。
	//	activeNode = aiTree->ActiveNodeInference(behaviorData);
	//}
	// 現在実行するノードがあれば
	//if (activeNode != nullptr)
	//{
	//	 ビヘイビアツリーからノードを実行。
	//	activeNode = aiTree->Run(activeNode, behaviorData, elapsedTime);
	//}

	// ステータス更新
	UpdateStatus(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// 武器のアタッチメント処理
	WeaponAttachment();

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

	renderer->Draw(ShaderId::PBR, weapon[0].model);

	if(!weapon->LeftHandInvincible) renderer->Draw(ShaderId::PBR, weapon[1].model);
}

// GUI描画
void Enemy::DrawGUI()
{
	ImGui::Begin("Enemy");
	{
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

		if (ImGui::Button(u8"左手武器表示"))
		{
			weapon->LeftHandInvincible = !weapon->LeftHandInvincible;
		}

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

		// 武器の位置
		if (ImGui::CollapsingHeader("Weapon Attachment")) {
			for (int i = 0; i < 2; ++i) {
				ImGui::PushID(i);
				ImGui::Text(i == 0 ? "Right Hand" : "Left Hand");
				ImGui::DragFloat3("Pos", &weapon[i].position.x, 0.01f);
				ImGui::DragFloat3("Ang", &weapon[i].angle.x, 0.01f);
				ImGui::DragFloat3("Scale", &weapon[i].scale.x, 0.01f);
				ImGui::DragFloat3("Weapon HitPos", &weapon[i].weaponHitOffset.x, 0.01f);
				ImGui::DragFloat3("Weapon HitAng", &weapon[i].weaponAngleOffset.x, 0.01f);
				ImGui::DragFloat("Weapon HitRad", &weapon[i].weaponRadius, 0.01f);
				ImGui::DragFloat("Weapon HitHeight", &weapon[i].weaponHeight, 0.01f);
				ImGui::PopID();
			}
		}
	}
	ImGui::End();

	ImGui::Begin("Enemy Attack Sequencer");
	{
		auto& AnimSequence = GetAnimSequence();
		auto& manager = AnimationStateManager<EnemyAnimationState>::Instance();

		float totalSec = AnimSequence.GetAnimationLength(AnimSequence.currentState);
		ImGui::Text(u8"総秒数: %.2f秒  (バーの数値 ÷ 100 = 秒)", totalSec);
			auto& tracks = AnimSequence.CurrentTracks();
		if (selectedEntry >= 0)
		{
			if (selectedEntry < (int)tracks.size())
			{
				auto& t = tracks[selectedEntry];
				ImGui::Text(u8"選択中: %.2f秒 〜 %.2f秒",
					t.GetStartSeconds(), t.GetEndSeconds());

				// TrackType の変更
				const char* trackTypeItems[] = { "HitBox", "Effect", "Sound" };
				int trackTypeIndex = (int)t.type;
				if (ImGui::Combo(u8"タイプ", &trackTypeIndex, trackTypeItems, IM_ARRAYSIZE(trackTypeItems)))
				{
					t.type = (TrackType)trackTypeIndex;
				}

				// HandType の変更（HitBox のときだけ表示）
				if (t.type == TrackType::HitBox)
				{
					const char* handItems[] = { "None", "RightHand", "LeftHand", "Both", "Body" };
					int handIndex = (int)t.hand;
					if (ImGui::Combo(u8"手", &handIndex, handItems, IM_ARRAYSIZE(handItems)))
					{
						t.hand = (HandType)handIndex;
					}

					if (t.hand == HandType::Body)
					{
						char boneBuf[128];
						strncpy_s(boneBuf, t.boneName.c_str(), sizeof(boneBuf));
						if (ImGui::InputText(u8"ボーン名", boneBuf, sizeof(boneBuf)))
							t.boneName = boneBuf;
						ImGui::DragFloat(u8"球の半径", &t.sphereRadius, 0.01f, 0.1f, 5.0f);
						ImGui::DragFloat3(u8"オフセット", &t.sphereOffset.x, 0.01f);
					}

					ImGui::DragFloat(u8"ダメージ倍率", &t.damageRate, 0.01f, 0.1f, 5.0f);
					ImGui::DragFloat(u8"無敵時間", &t.invincible, 0.01f);
					ImGui::DragFloat(u8"削り値", &t.poiseRate, 0.01f);
				}
			}
		}

		// 選択中のステートにトラックを追加するボタン
		if (ImGui::Button(u8"+ 追加"))
		{
			auto& tracks = AnimSequence.attackData[AnimSequence.currentState];
			int newIndex = (int)tracks.size() + 1; // 現在の数+1が新しい番号
			std::string label = u8"当たり判定 " + std::to_string(newIndex);
			tracks.push_back({ 0, 50, label, 0xFF0000FF, TrackType::HitBox, HandType::RightHand });
		}

		ImGui::SameLine();
		// 選択中のステートにトラックを削除するボタン
		if (ImGui::Button(u8"- 削除"))
		{
			if (selectedEntry >= 0 && selectedEntry < (int)tracks.size())
			{
				tracks.erase(tracks.begin() + selectedEntry);
				animSequence.RenumberTracks(tracks);
				selectedEntry = -1;
			}
		}


		// 保存・読み込みボタン
		if (ImGui::Button(u8"保存"))
			AnimSequence.Save("Data/Json/Enemy/AttackData/AttackSequence.json");
		ImGui::SameLine();
		if (ImGui::Button(u8"読み込み"))
			AnimSequence.Load("Data/Json/Enemy/AttackData/AttackSequence.json");

		if (ImGui::CollapsingHeader(u8"Animation List", ImGuiTreeNodeFlags_DefaultOpen)) 
		{
			// 高さ200pxのスクロールエリアを作成
			if (ImGui::BeginChild("AnimList", ImVec2(0, 200), true))
			{
				for (auto& [state, tracks] : AnimSequence.attackData)
				{
					const AnimationConfig* config = manager.GetConfig(state);

					// 横に並べず、あえて縦に並べる（選択しやすいため）
					// 現在選択中のアニメーションをハイライトすると分かりやすい
					bool is_selected = (AnimSequence.currentState == state);
					if (ImGui::Selectable(config->animationName.c_str(), is_selected))
					{
						AnimSequence.currentState = state;
						ChangeAnimationState(state);
					}
				}
			}
			ImGui::EndChild();
		}

		if (AnimSequence.currentState == GetCurrentState())
		{
			currentFrame = (int)(GetCurrentAnimationSeconds() * 144);
		}

		ImSequencer::Sequencer(
			&AnimSequence,
			&currentFrame,
			&sequencerExpanded,
			&selectedEntry,
			&firstFrame,
			ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME
		);
	}
	ImGui::End();

}

// デバックプリミティブ描画
void Enemy::RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox)
{
	// 敵の当たり判定
	{
		DirectX::XMFLOAT4X4 capsuleTransform;
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + debugOffset, position.z);
		DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

		renderer->DrawCapsule(capsuleTransform, radius, height, { 1, 1, 0, 1 });
	}

	// 武器の当たり判定
	if (showWeaponHitBox)
	{
		float currentSec = GetCurrentAnimationSeconds();
		auto currentState = GetCurrentState();

		for (int i = 0; i < 2; ++i)
		{
			HandType currentHand = (i == 0) ? HandType::RightHand : HandType::LeftHand;

			if (i == 0 && weapon[i].RightHandInvincible)
			{
				if (!GetAnimSequence().IsHitActive(currentState, currentSec, currentHand))
					continue;
			}
			if (i == 1 && weapon[i].LeftHandInvincible)
			{
				if (!GetAnimSequence().IsHitActive(currentState, currentSec, currentHand))
					continue;
			}

			DirectX::XMFLOAT4X4 weaponTransform;

			DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon[i].transform);

			// 武器の現在の位置と回転だけを取り出す（スケールを無視する）
			DirectX::XMVECTOR scale, rot, pos;
			DirectX::XMMatrixDecompose(&scale, &rot, &pos, weaponWorld);
			DirectX::XMMATRIX baseMatrix = DirectX::XMMatrixRotationQuaternion(rot) * DirectX::XMMatrixTranslationFromVector(pos);

			DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
				weapon[i].angle.x + weapon[i].weaponAngleOffset.x,
				weapon[i].angle.y + weapon[i].weaponAngleOffset.y,
				weapon[i].angle.z + weapon[i].weaponAngleOffset.z);
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
				weapon[i].position.x + weapon[i].weaponHitOffset.x,
				weapon[i].position.y + weapon[i].weaponHitOffset.y,
				weapon[i].position.z + weapon[i].weaponHitOffset.z);
			DirectX::XMMATRIX WorldWeapon = R * T * baseMatrix;
			DirectX::XMStoreFloat4x4(&weaponTransform, WorldWeapon);

			renderer->DrawCapsule(weaponTransform, weapon[i].weaponRadius, weapon[i].weaponHeight, { 1, 0, 0, 1 });
		}

	}
		// 武器の攻撃の当たり判定
		{
			for (auto& info : GetActiveSphereHits())
			{
				renderer->DrawSphere(info.position, info.radius, { 1.0f, 0.0f, 1.0f, 1.0f });
			}
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

// 武器の位置を取得
DirectX::XMFLOAT3 Enemy::GetWeaponPosition(int index) const
{
	DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon[index].transform);

	// スケールを除去
	DirectX::XMVECTOR scale, rot, pos;
	DirectX::XMMatrixDecompose(&scale, &rot, &pos, weaponWorld);
	DirectX::XMMATRIX baseMatrix =
		DirectX::XMMatrixRotationQuaternion(rot) *
		DirectX::XMMatrixTranslationFromVector(pos);

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
		weapon[index].angle.x + weapon[index].weaponAngleOffset.x,
		weapon[index].angle.y + weapon[index].weaponAngleOffset.y,
		weapon[index].angle.z + weapon[index].weaponAngleOffset.z);

	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
		weapon[index].position.x + weapon[index].weaponHitOffset.x,
		weapon[index].position.y + weapon[index].weaponHitOffset.y,
		weapon[index].position.z + weapon[index].weaponHitOffset.z);

	DirectX::XMMATRIX finalMatrix = R * T * baseMatrix;

	// 位置を取り出す
	DirectX::XMFLOAT3 result;
	result.x = finalMatrix.r[3].m128_f32[0];
	result.y = finalMatrix.r[3].m128_f32[1];
	result.z = finalMatrix.r[3].m128_f32[2];
	return result;
}

// 武器の向きを取得する
DirectX::XMFLOAT3 Enemy::GetWeaponDirection(int index) const
{
	// transformから上方向ベクトルを取得する
	DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon[index].transform);

	// 回転値オフセットを適用
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
		weapon[index].weaponAngleOffset.x,
		weapon[index].weaponAngleOffset.y,
		weapon[index].weaponAngleOffset.z
	);

	DirectX::XMMATRIX finalMatrix = rotation * weaponWorld;

	DirectX::XMVECTOR dir = DirectX::XMVectorSet(
		finalMatrix.r[1].m128_f32[0],
		finalMatrix.r[1].m128_f32[1],
		finalMatrix.r[1].m128_f32[2],
		0.0f
	);

	// 正規化して長さを1にする
	dir = DirectX::XMVector3Normalize(dir);

	DirectX::XMFLOAT3 Direction;
	DirectX::XMStoreFloat3(&Direction, dir);
	return Direction;
}

std::vector<Enemy::SphereHitInfo> Enemy::GetActiveSphereHits() const
{
	std::vector<SphereHitInfo> result;
	float currentSec = GetCurrentAnimationSeconds();
	auto state = GetCurrentState();

	auto it = animSequence.attackData.find(state);
	if (it == animSequence.attackData.end()) return result;

	for (auto& track : it->second)
	{
		if (track.type != TrackType::HitBox) continue;
		if (track.hand != HandType::Body) continue;
		if (currentSec < track.GetStartSeconds() || currentSec > track.GetEndSeconds()) continue;

		SphereHitInfo info;
		info.radius = track.sphereRadius;

		// ボーン名が指定されていなければボーン位置を使う
		if (!track.boneName.empty())
		{
			int nodeIndex = enemy->GetNodeIndex(track.boneName.c_str());
			if (nodeIndex >= 0)
			{
				auto& node = enemy->GetNodes()[nodeIndex];
				DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&node.worldTransform);

				// オフセットをワールド空間に変換して球の位置を求める
				DirectX::XMVECTOR pos = DirectX::XMVector3Transform(
					DirectX::XMLoadFloat3(&track.sphereOffset), world);
				DirectX::XMStoreFloat3(&info.position, pos);
			}
			else
			{
				info.position = position;
			}
		}
		else
		{
			// ボーン名無しの場合敵の位置＋offset
			info.position = {
				position.x + track.sphereOffset.x,
				position.y + track.sphereOffset.y,
				position.z + track.sphereOffset.z
			};
		}
		result.push_back(info);
	}

	return result;
}

// アニメーション更新処理
void Enemy::UpdateStateTransitions(float elapsedTime)
{
	switch (currentState)
	{
	case EnemyAnimationState::Idle:
		if (GetAsyncKeyState('1') & 0x8000) ChangeAnimationState(EnemyAnimationState::Light_Attack_01);
		if (GetAsyncKeyState('2') & 0x8000) ChangeAnimationState(EnemyAnimationState::Skill_DoubleSwings_Root);
		if (GetAsyncKeyState('3') & 0x8000) ChangeAnimationState(EnemyAnimationState::Skill_EndlessStabs);

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
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Light_Attack_03:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Heavy_Attack_01:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Heavy_Attack_02:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Dodge_FU:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Grab_Fall:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Roar:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_BlockBreaker:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_DoubleSwings_Root:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_EndlessStabs:
		weapon->LeftHandInvincible = false;

		if (IsAnimationOutTimeRange(3.248)) weapon->LeftHandInvincible = true;

		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_QuickStab:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_HeavyStomp:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_Leaping:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_ShoulderBarge_Root:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_UpperCut:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
		break;
	case EnemyAnimationState::Skill_WieldDagger:
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);
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

// downしたときに呼ばれる
void Enemy::OnDown()
{
	Ondown = true;
}

// 武器のアタッチメント処理
void Enemy::WeaponAttachment()
{
	const char* HandName[2] = { "Bip001-R-Hand", "Bip001-L-Hand" };

	for (int i = 0; i < 2; ++i)
	{
		// 武器のローカル行列を計算
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(weapon[i].scale.x, weapon[i].scale.y, weapon[i].scale.z);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(weapon[i].angle.x, weapon[i].angle.y, weapon[i].angle.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(weapon[i].position.x, weapon[i].position.y, weapon[i].position.z);
		DirectX::XMMATRIX weaponLocal = S * R * T;

		// モデルから両手のノードを検索する
		for (const Model::Node& node : enemy->GetNodes())
		{
			if (strcmp(node.name.c_str(), HandName[i]) == 0)
			{
				// 右手ノードと武器のローカル行列から武器のワールド行列を求める
				DirectX::XMMATRIX rightHandGlobal = DirectX::XMLoadFloat4x4(&node.globalTransform);
				DirectX::XMMATRIX playerWorld = DirectX::XMLoadFloat4x4(&GetTransform());
				DirectX::XMMATRIX weaponWorld = weaponLocal * rightHandGlobal * playerWorld;
				DirectX::XMStoreFloat4x4(&weapon[i].transform, weaponWorld);
				weapon[i].model->UpdateTransform(weapon[i].transform);
				break;
			}
		}
	}
}
