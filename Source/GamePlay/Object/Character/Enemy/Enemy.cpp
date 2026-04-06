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


// デストラクタ
Enemy::~Enemy()
{
	delete aiTree;
	delete behaviorData;
}

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
	weapon[0].position = { -0.06, 0.0, -0.01 };
	weapon[0].angle = { 9.54, 9.48, -15.74 };
	weapon[0].weaponHitOffset = { -0.89, -0.11, -0.01 };
	weapon[0].weaponAngleOffset = { 1.6, 6.76, 1.06 };
	weapon[0].weaponRadius = 0.59f;
	weapon[0].weaponHeight = 1.7f;

	weapon[1].position = { -0.03, 0.01, 0.01 };
	weapon[1].angle = { -3.57, 3.39, 3.12 };
	weapon[1].weaponHitOffset = { -0.82, -0.1, -0.04 };
	weapon[1].weaponAngleOffset = { 1.96, 4.3, 0.17 };
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

	// 攻撃とかの情報を初期化
	InitializeAttackData();

	// ビヘイビアツリーの設定
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree();

	// Root
	aiTree->AddNode("", "Root", 0, BehaviorTree::SelectRule::Priority, nullptr, nullptr);

	aiTree->AddNode("Root", "Attack" , 1, BehaviorTree::SelectRule::Priority, new AttackJudgment(this), new AttackAction(this));
	aiTree->AddNode("Root", "Pursuit", 2, BehaviorTree::SelectRule::Priority, new PursuitJudgment(this), new PursuitAction(this));
	aiTree->AddNode("Root", "Wander", 3, BehaviorTree::SelectRule::Priority, new WanderJudgment(this), new WanderAction(this));
	aiTree->AddNode("Root", "Idle", 4, BehaviorTree::SelectRule::Priority, /*new IdleJudgment(this)*/nullptr, new IdleAction(this));
}

// 攻撃とかの情報を初期化
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

		animSequence.attackData[EnemyAnimationState::Dodge_FU] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::Body }
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

	// 始動技の初期化
	firstAttackList.push_back({ EnemyAnimationState::Light_Attack_01, 0.0f, Short_Distance });
	firstAttackList.push_back({ EnemyAnimationState::Skill_QuickStab, 0.0f, Short_Distance });
	firstAttackList.push_back({ EnemyAnimationState::Skill_EndlessStabs, 0.0f, Short_Distance });
	firstAttackList.push_back({ EnemyAnimationState::Skill_WieldDagger, 0.0f, Short_Distance });

	firstAttackList.push_back({ EnemyAnimationState::Skill_BlockBreaker, Short_Distance, Long_Distance });
	firstAttackList.push_back({ EnemyAnimationState::Skill_HeavyStomp, Short_Distance, Long_Distance });
	firstAttackList.push_back({ EnemyAnimationState::Skill_DoubleSwings_Root, Short_Distance, Long_Distance });
	firstAttackList.push_back({ EnemyAnimationState::Skill_ShoulderBarge_Root, Short_Distance, Long_Distance });


	// コンボとその派生先の初期化

	// 近距離
	attackComboMap[EnemyAnimationState::Light_Attack_01] = {
		{ EnemyAnimationState::Light_Attack_02, 0.0f, Short_Distance, 80 },
		{ EnemyAnimationState::Dodge_FU, 0.0f, Short_Distance, 80 },
		{ EnemyAnimationState::Dodge_Backward, 0.0f, Short_Distance, 50 },
	};
	attackComboMap[EnemyAnimationState::Light_Attack_02] = {
		{ EnemyAnimationState::Light_Attack_03, 0.0f, Short_Distance, 80 },
		{ EnemyAnimationState::Heavy_Attack_02, 0.0f, Short_Distance, 40 },
	};
	attackComboMap[EnemyAnimationState::Dodge_FU] = {
		{ EnemyAnimationState::Light_Attack_03, 0.0f, Short_Distance, 80 },
	};

	attackComboMap[EnemyAnimationState::Light_Attack_03] = {
		{ EnemyAnimationState::Heavy_Attack_01, 0.0f, Short_Distance, 40 },
	};

	// 中距離

	attackComboMap[EnemyAnimationState::Dodge_Backward] = {
		{ EnemyAnimationState::Skill_Leaping, 0.0f, Middle_Distance, 50 },
		{ EnemyAnimationState::Skill_UpperCut, 0.0f, Short_Distance, 50 },
		{ EnemyAnimationState::Dodge_Backward, 0.0f, Short_Distance, 50 },
	};
}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	//現在実行されているノードが無ければ
	if (activeNode == nullptr)
	{
		//次に実行するノードを推論する。
		activeNode = aiTree->ActiveNodeInference(behaviorData);
	}
	//現在実行するノードがあれば
	if (activeNode != nullptr)
	{
		//ビヘイビアツリーからノードを実行。
		activeNode = aiTree->Run(activeNode, behaviorData, elapsedTime);
	}

	if (attackCoolTimer > 0.0f) attackCoolTimer -= elapsedTime;

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

	// エリア移動制限
	AreaRestriction();

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
	std::string str = "";
	if (activeNode != nullptr)
	{
		str = activeNode->GetName();
	}

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
		ImGui::Text("attackCoolTimer: %f.0", attackCoolTimer);
		ImGui::Separator();
		ImGui::Text("runTimer: %f.0", runTimer);
		ImGui::Separator();
		ImGui::Text(u8"Behavior　%s", str.c_str());


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

	// 武器以外の攻撃の当たり判定
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
			Short_Distance, thickness, { 1.0f, 0.0f, 0.0f, 1.0f }
		);

		// 中距離 (例: 10.0m) - 黄色
		renderer->DrawCylinder(
			{ position.x, circleY, position.z },
			Middle_Distance, thickness, { 1.0f, 1.0f, 0.0f, 1.0f }
		);

		// 遠距離 (例: 15.0m) - 緑色
		renderer->DrawCylinder(
			{ position.x, circleY, position.z },
			Long_Distance, thickness, { 0.0f, 1.0f, 0.0f, 1.0f }
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

// ターゲットの座標に向かって旋回する
void Enemy::TurnToPosition(float elapsedTime, const DirectX::XMFLOAT3& targetPos)
{
	DirectX::XMFLOAT3 myPos = GetPosition();

	// プレイヤーへの方向ベクトルを計算
	float vx = targetPos.x - myPos.x;
	float vz = targetPos.z - myPos.z;
	float len = sqrtf(vx * vx + vz * vz);

	// 距離が極端に近くない場合のみ回転
	if (len > 0.001f)
	{
		vx /= len;
		vz /= len;

		// すでに持っている Turn 関数を呼び出す
		// 第4引数は Enemy が持っている旋回速度
		this->Turn(elapsedTime, vx, vz, turnSpeed);
	}
}

void Enemy::TurnToPosition(float elapsedTime)
{
	Player& player = Player::Instance();
	TurnToPosition(elapsedTime, player.GetPosition());
}

bool Enemy::IsFacingTarget(const DirectX::XMFLOAT3& targetPos, float epsilonDegree)
{
	// 1. ターゲットへの方向ベクトル
	float vx = targetPos.x - GetPosition().x;
	float vz = targetPos.z - GetPosition().z;
	float targetAngle = atan2f(vx, vz); // 目標の角度

	// 2. 現在の自分の角度（Rotation.y）
	float currentAngle = GetAngle().y;

	// 3. 角度の差を計算（-PI 〜 PI の範囲に補正するのが理想）
	float diff = targetAngle - currentAngle;
	while (diff > DirectX::XM_PI) diff -= DirectX::XM_2PI;
	while (diff < -DirectX::XM_PI) diff += DirectX::XM_2PI;

	// 4. 差分が指定した角度（epsilonDegree）以内なら true
	return fabsf(diff) < DirectX::XMConvertToRadians(epsilonDegree);
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

// 技の派生があるか確認する関数
EnemyAnimationState Enemy::DecideNextAttack(EnemyAnimationState currentState)
{
	// 今の技から派生できるものがあるかを確認
	if (attackComboMap.count(currentState) == 0) return (EnemyAnimationState)-1;

	// プレイヤーとの距離を取得
	float dist = GetDistanceToPlayer();
	auto& derivations = attackComboMap[currentState];

	// 候補をいったん保存する
	std::vector<EnemyAnimationState> candidates;

	for (const auto& der : derivations)
	{
		// 技の範囲内か
		if (dist >= der.minDistance && dist <= der.maxDistance)
		{
			if ((rand() % 100) < der.probability)
			{
				candidates.push_back(der.nextState);
			}
		}
	}

	// 候補が無ければIdleを返す
	if (candidates.empty()) return (EnemyAnimationState)-1;

	// 候補の中からされにランダムに一つ選ぶ
	return candidates[rand() % candidates.size()];
}

// スタンプ攻撃
void Enemy::HeavyStompAttack()
{
	moveSpeed = 10;

	// プレイヤーを取得
	Player& player = Player::Instance();

	// 位置情報を取得する
	DirectX::XMFLOAT3 bossPos = GetPosition();
	DirectX::XMFLOAT3 playerPos = player.GetPosition();

	// プレイヤーへの方向ベクトルを計算
	float vx = playerPos.x - bossPos.x;
	float vz = playerPos.z - bossPos.z;
	float distance = GetDistanceToPlayer();


	if (IsAnimationInTimeRange(0.0f, 0.843f))
	{
		Move(vx, vz, moveSpeed);
	}
	else
	{
		moveSpeed = 0;
	}
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

// 始動技を決める関数
EnemyAnimationState Enemy::DecideFirstAttack()
{
	float dist = GetDistanceToPlayer();
	std::vector<EnemyAnimationState> validAttacks;

	// 現在の距離に適合する技をすべてリストアップ
	for (auto& attack : firstAttackList) {
		if (dist >= attack.minRange && dist <= attack.maxRange) {
			validAttacks.push_back(attack.state);
		}
	}

	// もし候補が一つもなかったら、とりあえず基本攻撃を返すか -1 を返す
	if (validAttacks.empty()) {
		return (EnemyAnimationState)-1; // または (EnemyAnimationState)-1;
	}

	// 候補の中からランダムに1つ選んで返す
	int randomIndex = rand() % validAttacks.size();
	return validAttacks[randomIndex];
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
		if (IsAnimationFinished()) ChangeAnimationState(EnemyAnimationState::Idle);

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
	//const char* HandName[2] = { "Bip001-R-Hand", "Bip001-L-Hand" };
	const char* HandName[2] = { "Bip001-Prop1", "Bip001-Prop2" };

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