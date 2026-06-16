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

	enemy = std::make_shared<Model>(device, "Data/Model/Enemy/Boss2/SevarogBloodred.gltf");

	// 敵モデルの設定
	position = { 0, 0, 10 };
	angle = {0, 3, 0};
	scale.x = scale.y = scale.z = 2.0f;
	weight = 100.0f;
	radius = 1.0f;
	height = 1.6f;
	debugOffset = 0.8;

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
	ChangeAnimationState("Idle");

	// 攻撃とかの情報を初期化

	// ビヘイビアツリーの設定
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree();

	// Root
	aiTree->AddNode("", "Root", 0, BehaviorTree::SelectRule::Priority, nullptr, nullptr);

	aiTree->AddNode("Root", "Attack" , 1, BehaviorTree::SelectRule::Priority, new AttackJudgment(this), new AttackAction(this));
	aiTree->AddNode("Root", "Pursuit", 2, BehaviorTree::SelectRule::Priority, new PursuitJudgment(this), new PursuitAction(this));
	aiTree->AddNode("Root", "Wander", 3, BehaviorTree::SelectRule::Priority, new WanderJudgment(this), new WanderAction(this));
	aiTree->AddNode("Root", "Idle", 4, BehaviorTree::SelectRule::Priority, nullptr, new IdleAction(this));
}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	////現在実行されているノードが無ければ
	//if (activeNode == nullptr)
	//{
	//	//次に実行するノードを推論する。
	//	activeNode = aiTree->ActiveNodeInference(behaviorData);
	//}
	////現在実行するノードがあれば
	//if (activeNode != nullptr)
	//{
	//	//ビヘイビアツリーからノードを実行。
	//	activeNode = aiTree->Run(activeNode, behaviorData, elapsedTime);
	//}

	if (attackCoolTimer > 0.0f) attackCoolTimer -= elapsedTime;

	// ステータス更新
	UpdateStatus(elapsedTime);

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// 武器のアタッチメント処理
	//WeaponAttachment();

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
				ImGui::DragFloat3("Pos", &weapon.position.x, 0.01f);
				ImGui::DragFloat3("Ang", &weapon.angle.x, 0.01f);
				ImGui::DragFloat3("Scale", &weapon.scale.x, 0.01f);
				ImGui::DragFloat3("Weapon HitPos", &weapon.weaponHitOffset.x, 0.01f);
				ImGui::DragFloat3("Weapon HitAng", &weapon.weaponAngleOffset.x, 0.01f);
				ImGui::DragFloat("Weapon HitRad", &weapon.weaponRadius, 0.01f);
				ImGui::DragFloat("Weapon HitHeight", &weapon.weaponHeight, 0.01f);
		}
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
DirectX::XMFLOAT3 Enemy::GetWeaponPosition() const
{
	DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

	// スケールを除去
	DirectX::XMVECTOR scale, rot, pos;
	DirectX::XMMatrixDecompose(&scale, &rot, &pos, weaponWorld);
	DirectX::XMMATRIX baseMatrix =
		DirectX::XMMatrixRotationQuaternion(rot) *
		DirectX::XMMatrixTranslationFromVector(pos);

	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
		weapon.angle.x + weapon.weaponAngleOffset.x,
		weapon.angle.y + weapon.weaponAngleOffset.y,
		weapon.angle.z + weapon.weaponAngleOffset.z);

	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
		weapon.position.x + weapon.weaponHitOffset.x,
		weapon.position.y + weapon.weaponHitOffset.y,
		weapon.position.z + weapon.weaponHitOffset.z);

	DirectX::XMMATRIX finalMatrix = R * T * baseMatrix;

	// 位置を取り出す
	DirectX::XMFLOAT3 result;
	result.x = finalMatrix.r[3].m128_f32[0];
	result.y = finalMatrix.r[3].m128_f32[1];
	result.z = finalMatrix.r[3].m128_f32[2];
	return result;
}

// 武器の向きを取得する
DirectX::XMFLOAT3 Enemy::GetWeaponDirection() const
{
	// transformから上方向ベクトルを取得する
	DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

	// 回転値オフセットを適用
	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationRollPitchYaw(
		weapon.weaponAngleOffset.x,
		weapon.weaponAngleOffset.y,
		weapon.weaponAngleOffset.z
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
std::string Enemy::DecideNextAttack(std::string currentState)
{
	// 今の技から派生できるものがあるかを確認
	if (attackComboMap.count(currentState) == 0) return "";

	// プレイヤーとの距離を取得
	float dist = GetDistanceToPlayer();
	auto& derivations = attackComboMap[currentState];

	// 候補をいったん保存する
	std::vector<std::string> candidates;

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
	if (candidates.empty()) return "";

	// 候補の中からされにランダムに一つ選ぶ
	return candidates[rand() % candidates.size()];
}

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
// 始動技を決める関数
std::string Enemy::DecideFirstAttack()
{
	float dist = GetDistanceToPlayer();
	std::vector<std::string> validAttacks;

	// 現在の距離に適合する技をすべてリストアップ
	for (auto& attack : firstAttackList) {
		if (dist >= attack.minRange && dist <= attack.maxRange) {
			validAttacks.push_back(attack.state);
		}
	}

	// もし候補が一つもなかったら、とりあえず基本攻撃を返すか -1 を返す
	if (validAttacks.empty()) {
		return ""; // または (EnemyAnimationState)-1;
	}

	// 候補の中からランダムに1つ選んで返す
	int randomIndex = rand() % validAttacks.size();
	return validAttacks[randomIndex];
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
	const char* HandName = "root_weapon_r";

	// 武器のローカル行列を計算
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(weapon.scale.x, weapon.scale.y, weapon.scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(weapon.angle.x, weapon.angle.y, weapon.angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(weapon.position.x, weapon.position.y, weapon.position.z);
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
			DirectX::XMStoreFloat4x4(&weapon.transform, weaponWorld);
			weapon.model->UpdateTransform(weapon.transform);
			break;
		}
	}
}