#include "Enemy.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"
#include "System/Audio/Audio.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
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

	enemy = std::make_shared<Model>(device, "Data/Model/Enemy/Boss/SKM_Hairy_Beast.gltf");

	weapon[0].model = std::make_shared<Model>(device, "Data/Model/Weapon/Enemy/Boss/SKM_Hairy_Beast_Hammer.gltf");

	// 敵モデルの設定
	position = { 0, 0, 10 };
	angle = {0, 3, 0};
	scale.x = scale.y = scale.z = 6.0f;
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
	LoadAnimationData("Data/Json/Enemy/Enemy_AnimationGraph.json");
	enemy->GetNodePoses(nodePoses);
	enemy->GetNodePoses(oldNodePoses);
	rootMotionNodeName = "root";
	upperBodyNodeName = "pelvis";
	ChangeAnimationState("Idle_2");
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree();

	// Root
	//aiTree->AddNode("", "Root", 0, BehaviorTree::SelectRule::Priority, nullptr, nullptr);

	//aiTree->AddNode("Root", "Attack" , 1, BehaviorTree::SelectRule::Priority, new AttackJudgment(this), new AttackAction(this));
	//aiTree->AddNode("Root", "Pursuit", 2, BehaviorTree::SelectRule::Priority, new PursuitJudgment(this), new PursuitAction(this));
	//aiTree->AddNode("Root", "Wander", 3, BehaviorTree::SelectRule::Priority, new WanderJudgment(this), new WanderAction(this));
	//aiTree->AddNode("Root", "Idle", 4, BehaviorTree::SelectRule::Priority, /*new IdleJudgment(this)*/nullptr, new IdleAction(this));

	// モデルからアニメーション名一覧を取得してエディターに渡す
	std::vector<std::string> animNames;
	for (auto& anim : enemy->GetAnimations())
		animNames.push_back(anim.name);
	btEditor.SetAnimationList(animNames);

	btGraph.Load("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	LoadAnimationData("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	btEditor.ApplyToTree(aiTree, this, btGraph);
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

	animSequence.Update(currentState, GetCurrentAnimationSeconds());

	UpdateSounds(this->currentState);
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

	btEditor.Draw(btGraph);
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
		for (int i = 0; i < 2; ++i)
		{
			HandType currentHand = (i == 0) ? HandType::RightHand : HandType::LeftHand;

			// 新しいシーケンサーから、今有効な判定（Range）を探す
			bool isHit = false;
			for (auto& data : animSequence.GetAnimations()) {
				if (data.name == currentState) {
					for (auto& r : data.ranges) {
						// その手の判定が存在し、かつ現在時刻が有効範囲内(GetRangeがtrue)か？
						if (r.hand == currentHand && animSequence.GetRange(r.name)) {
							isHit = true;
							break;
						}
					}
					break;
				}
			}

			if (i == 0 && weapon[i].RightHandInvincible && !isHit) continue;
			if (i == 1 && weapon[i].LeftHandInvincible && !isHit) continue;

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

// 球の当たり判定
std::vector<Enemy::SphereHitInfo> Enemy::GetActiveSphereHits() const
{
	std::vector<SphereHitInfo> result;
	auto state = GetCurrentState();

	for (auto& data : animSequence.GetAnimations())
	{
		if (data.name != state) continue;

		for (auto& r : data.ranges)
		{
			if (r.hand != HandType::Body) continue;
			if (!animSequence.GetRange(r.name)) continue; // 今フレームで有効な範囲かチェック

			SphereHitInfo info;
			info.radius = r.sphereRadius;

			if (!r.boneName.empty())
			{
				int nodeIndex = enemy->GetNodeIndex(r.boneName.c_str());
				if (nodeIndex >= 0)
				{
					auto& node = enemy->GetNodes()[nodeIndex];
					DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&node.worldTransform);
					DirectX::XMVECTOR pos = DirectX::XMVector3Transform(
						DirectX::XMLoadFloat3(&r.sphereOffset), world);
					DirectX::XMStoreFloat3(&info.position, pos);
				}
				else
				{
					info.position = position;
				}
			}
			else
			{
				info.position = {
					position.x + r.sphereOffset.x,
					position.y + r.sphereOffset.y,
					position.z + r.sphereOffset.z
				};
			}
			result.push_back(info);
		}
		break;
	}
	return result;
}

// アニメーション更新処理
void Enemy::UpdateStateTransitions(float elapsedTime)
{
}

// downしたときに呼ばれる
void Enemy::OnDown()
{
	Ondown = true;
}

// アニメーションのコールバック関数
void Enemy::OnStateChanged(const std::string& oldState, const std::string& newState)
{
}

// サウンドを流す
void Enemy::UpdateSounds(const std::string& state)
{
	for (auto& data : animSequence.GetAnimations())
	{
		if (data.name != state) continue;

		for (auto& e : data.events)
		{
			if (e.type != EventType::Sound) continue;

			// 今フレームで発火したか？
			if (animSequence.GetEvent(e.name)) 
			{
				AudioSource* se = GetOrLoadSound(e.soundName);
				if (se) {
					se->Play(false);
				}
			}
		}
		break;
	}
}

// 音を取得（無ければ自動ロード）する関数
AudioSource* Enemy::GetOrLoadSound(const std::string& soundName)
{
	// 空文字なら何もしない
	if (soundName.empty()) return nullptr;

	// 既にmapにあるかを確認（読み込み失敗したものも含めてキャッシュ）
	if (sounds.count(soundName)) {
		return sounds[soundName].get();
	}

	// パスを組み立てる
	std::string fileName = soundName;
	// もし入力に.wavが含まれていなければ足す
	if (fileName.find(".wav") == std::string::npos) {
		fileName += ".wav";
	}
	std::string path = "Data/Sound/" + fileName;

	// ロード試行
	auto source = Audio::Instance().LoadAudioSource(path.c_str());

	if (source) {
		sounds[soundName] = std::unique_ptr<AudioSource>(source);
		return sounds[soundName].get();
	}
	else {
		// 見つからなかった場合、次は探さないようにnullptrを保持（または警告を出すようにする）
		sounds[soundName] = nullptr;
		return nullptr;
	}
}

// 武器のアタッチメント処理
void Enemy::WeaponAttachment()
{
	//const char* HandName[2] = { "Bip001-R-Hand", "Bip001-L-Hand" };
	//const char* HandName[2] = { "Bip001-Prop1", "Bip001-Prop2" };
	const char* HandName = "Bip001-Prop1";

	// 武器のローカル行列を計算
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(weapon[0].scale.x, weapon[0].scale.y, weapon[0].scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(weapon[0].angle.x, weapon[0].angle.y, weapon[0].angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(weapon[0].position.x, weapon[0].position.y, weapon[0].position.z);
	DirectX::XMMATRIX weaponLocal = S * R * T;

	// モデルから両手のノードを検索する
	for (const Model::Node& node : enemy->GetNodes())
	{
		if (strcmp(node.name.c_str(), HandName) == 0)
		{
			// 右手ノードと武器のローカル行列から武器のワールド行列を求める
			DirectX::XMMATRIX rightHandGlobal = DirectX::XMLoadFloat4x4(&node.globalTransform);
			DirectX::XMMATRIX playerWorld = DirectX::XMLoadFloat4x4(&GetTransform());
			DirectX::XMMATRIX weaponWorld = weaponLocal * rightHandGlobal * playerWorld;
			DirectX::XMStoreFloat4x4(&weapon[0].transform, weaponWorld);
			weapon[0].model->UpdateTransform(weapon[0].transform);
			break;
		}
	}

	//for (int i = 0; i < 2; ++i)
	//{
	//	// 武器のローカル行列を計算
	//	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(weapon[i].scale.x, weapon[i].scale.y, weapon[i].scale.z);
	//	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(weapon[i].angle.x, weapon[i].angle.y, weapon[i].angle.z);
	//	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(weapon[i].position.x, weapon[i].position.y, weapon[i].position.z);
	//	DirectX::XMMATRIX weaponLocal = S * R * T;

	//	// モデルから両手のノードを検索する
	//	for (const Model::Node& node : enemy->GetNodes())
	//	{
	//		if (strcmp(node.name.c_str(), HandName[i]) == 0)
	//		{
	//			// 右手ノードと武器のローカル行列から武器のワールド行列を求める
	//			DirectX::XMMATRIX rightHandGlobal = DirectX::XMLoadFloat4x4(&node.globalTransform);
	//			DirectX::XMMATRIX playerWorld = DirectX::XMLoadFloat4x4(&GetTransform());
	//			DirectX::XMMATRIX weaponWorld = weaponLocal * rightHandGlobal * playerWorld;
	//			DirectX::XMStoreFloat4x4(&weapon[i].transform, weaponWorld);
	//			weapon[i].model->UpdateTransform(weapon[i].transform);
	//			break;
	//		}
	//	}
	//}
}