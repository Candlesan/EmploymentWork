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

	enemy = std::make_shared<Model>(device, "Data/Model/Player/Player/SK_SwordsmanGirl_02.gltf");

	// アニメーター生成
	animator = std::make_unique<Animator>();
	animator->Initialize(enemy);

	// 敵モデルの設定
	weight = 0.5f;
	radius = 0.4f;
	height = 1.6f;
	debugOffset = 0.8;

	position = { 0, 0, 10 };
	angle = { 0, 3, 0 };

	// パラメータ初期化
	moveSpeed = 7.0f;
	maxMoveSpeed = 7.0f;
	status.currentHp = 100.0f;
	status.maxHp = 100.0f;
	status.currentMagic = 100.0f;
	status.maxMagic = 100.0f;
	status.maxDefenseDurability = 100.0f;
	status.currentDefenseDurability = 100.0f;


	// アニメーション設定
	enemy->GetNodePoses(nodePoses);
	enemy->GetNodePoses(oldNodePoses);
	rootMotionNodeName = "root";
	upperBodyNodeName = "pelvis";
	behaviorData = new BehaviorData();
	aiTree = new BehaviorTree();
	AISwitch = false;

	// モデルからアニメーション名一覧を取得してエディターに渡す
	std::vector<std::string> animNames;
	for (auto& anim : enemy->GetAnimations())
		animNames.push_back(anim.name);
	btEditor.SetAnimationList(animNames);

	btGraph.Load("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	LoadAnimationData("Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json");
	btEditor.ApplyToTree(aiTree, this, btGraph);

	// シーケンサーのセーブデータ用パスを設定して読み込む
	std::string seqPath = "Data/Json/Enemy/AnimationSequencer/EnemyAnimationSequence.json";
	sequencerEditor.SetJsonPath(seqPath);
	sequencerEditor.SetWindowName("Enemy Sequencer");
	animSequence.Load(seqPath);

	ChangeAnimationState("Idle");

}

// 更新処理
void Enemy::Update(float elapsedTime)
{
	// プレビュー中はAIを止める
	if (!isPreviewMode && AISwitch)
	{
		//現在実行されているノードが無ければ
		if (activeNode == nullptr)
		{
			//次に実行するノードを推論する。
			activeNode = aiTree->ActiveNodeInference(behaviorData);
			if (activeNode != nullptr)
			{
				lastInferredNode = activeNode;
			}
		}
		//現在実行するノードがあれば
		if (activeNode != nullptr)
		{
			//ビヘイビアツリーからノードを実行。
			activeNode = aiTree->Run(activeNode, behaviorData, elapsedTime);
		}
	}
	else
	{
		// プレビュー中のアニメーション強制ループ
		if (GetCurrentAnimationSeconds() >= GetCurrentAnimationLength())
		{
			animationSeconds = 0.0f;
		}
	}

	// GUIの表示/非表示
	ShowDebugSwitch();

	// 魔法の更新処理
	magicManager.Update(elapsedTime);

	if (attackCoolTimer > 0.0f) attackCoolTimer -= elapsedTime;

	// ステータス更新
	UpdateStatus(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	// シーケンサーの更新
	animSequence.Update(currentState, GetCurrentAnimationSeconds());
	SetBaseSpeed(animSequence.GetSpeed());

	// 音の更新
	UpdateSounds(this->currentState);

	// 魔法更新処理
	magicManager.Update(elapsedTime);
	defenseMagic.Update(this, elapsedTime);

	// エリア移動制限
	AreaRestriction();

	// モデル更新処理
	UpdateTransform();
	enemy->UpdateTransform(transform);
}

// 描画処理
void Enemy::Render(RenderContext& rc, ModelRenderer* renderer)
{
	if(showEnemy)
	renderer->Draw(ShaderId::PBR, enemy);
}

// GUI描画
void Enemy::DrawGUI()
{
	// 実行中のAIのステートを表示する
	std::string str = "";
	if (activeNode != nullptr)
	{
		str = activeNode->GetName();
	}

	// 敵のパラメーターの表示/非表示
	if (showEnemyParameter)
	{
		ImGui::Begin(u8"エネミー");
		{
			ImGui::Checkbox(u8"敵の表示", &showEnemy);

			if (ImGui::Button(u8"HP全回復"))
			{
				status.currentHp = status.maxHp;
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"魔力全回復"))
			{
				status.currentMagic = status.maxMagic;
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"ガード耐久値全回復"))
			{
				status.currentDefenseDurability = status.maxDefenseDurability;
			}
			ImGui::Text(u8"体力 : %f.0", status.currentHp);
			ImGui::DragFloat(u8"体力", &status.currentHp, 0.1f);
			ImGui::Separator();

			ImGui::Text(u8"魔力 : %f.0", status.currentMagic);
			ImGui::Text(u8"魔力の回復速度 : %f.0", status.currentMagicRecoverSpeed);
			ImGui::Text(u8"魔力の回復する遅延 : %f.0", status.magicRecoverDelayTimer);
			ImGui::DragFloat(u8"魔力", &status.currentMagic, 0.1f);
			ImGui::DragFloat(u8"基本の魔力回復速度", &status.magicRecoverSpeed, 0.1f);
			ImGui::DragFloat(u8"魔力の回復する遅延", &magicRecoverDelay, 0.1f);
			if (ImGui::Button(u8"魔力消費"))
			{
				ConsumMagic(10.0f, magicRecoverDelay);
			}
			if(ImGui::Checkbox(u8"防御魔法展開", &status.isGuarding))
			{
				defenseMagic.SetIsActive(status.isGuarding);
			}
			ImGui::Separator();

			ImGui::Text(u8"ヒート値 : %f.0", status.currentHeat);
			ImGui::DragFloat(u8"ヒート値", &status.currentHeat, 0.1f);
			ImGui::DragFloat(u8"ヒート値増加量", &status.heatIncreaseSpeed, 0.1f);
			ImGui::DragFloat(u8"ヒート値減少量", &status.heatRecoverSpeed, 0.1f);
			ImGui::Checkbox(u8"連射魔法テスト", &status.isRapidFire);
			ImGui::Separator();

			ImGui::Text(u8"防御魔法の耐久値 : %f.0", status.currentDefenseDurability);
			ImGui::DragFloat(u8"防御魔法の魔力消費量", &status.defenseMagicCost, 0.1f);
			if (ImGui::Button(u8"耐久値減少"))
			{
				OnGuardHit(10.0f);
			}
			ImGui::Separator();

			ImGui::Text(u8"魔法クールダウン: %f", magicCoolDownTimer);
			ImGui::Separator();

			ImGui::Text(u8"ステート　%s", str.c_str());
			std::string animState = GetCurrentState();
			ImGui::Text(u8"AnimState: %s", animState.c_str());

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
		}
		ImGui::End();
	}

	// BTEditorの表示/非表示
	if (showbtEditor)
	{
		ImGui::Begin("BTEditor");
		if (ImGui::Button("AI:ON/OFF"))
		{
			AISwitch = !AISwitch;
		}
		btEditor.Draw(btGraph, aiTree, this);
		ImGui::End();

		if (btEditor.requestOpenSequencer)
		{
			isPreviewMode = true;
			editingState = btEditor.selectedAnimName;

			ChangeAnimationState(editingState); // アニメーションを対象の技に切り替え
			ImGui::SetWindowFocus("Enemy Sequencer");

			btEditor.requestOpenSequencer = false; // フラグを戻す
		}

		// プレビューモード用の小さな操作ウィンドウ
		if (isPreviewMode)
		{
			ImGui::Begin("Preview Timeline");

			if (ImGui::Button(u8"▶ ゲームに戻る"))
			{
				isPreviewMode = false;
				ClearActiveNode(); // AIのステートをリセットして最初から考え直させる
				ChangeAnimationState("Idle");
			}
			ImGui::SameLine();

			float length = GetCurrentAnimationLength();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[%s]", editingState.c_str());
			ImGui::SameLine();
			ImGui::Text("  Time: %.2f / %.2f s", animationSeconds, length);

			ImGui::SetNextItemWidth(-1);
			if (ImGui::SliderFloat("##animTime", &animationSeconds, 0.0f, length))
			{
				isBlending = false; // スライダー操作中はブレンドを切る
			}

			ImGui::End();
		}

		// シーケンサー本体の描画
		std::string displayState = isPreviewMode ? editingState : currentState;
		sequencerEditor.DrawEditor(
			animSequence,
			displayState,
			GetCurrentAnimationSeconds(),
			GetCurrentAnimationLength()
		);
	}
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

	if (defenseMagic.GetIsActive())
	{
		renderer->DrawSphere(defenseMagic.GetDefensePosition(), defenseMagic.GetDefenseRadius(), { 0, 0, 1, 1 });
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

	// 魔法の当たり判定描画
	magicManager.RenderDebugPrimitive(renderer);
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

// ターゲットの座標に向かって進む
void Enemy::MoveToTarget(float elapsedTime, const DirectX::XMFLOAT3& targetPos)
{
	// 位置を取得する
	DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR PPos = DirectX::XMLoadFloat3(&targetPos);
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(PPos, EPos);
	Vec = DirectX::XMVector3Normalize(Vec); // 正規化

	DirectX::XMFLOAT3 moveVec;
	DirectX::XMStoreFloat3(&moveVec, Vec);

	// ターゲットの方向を向く
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);

	//　その方向へ進む
	Move(moveVec.x, moveVec.z, moveSpeed);
}

void Enemy::MoveToTarget(float elapsedTime)
{
	Player& player = Player::Instance();
	MoveToTarget(elapsedTime, player.GetPosition());
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

// GUIの表示/非表示
void Enemy::ShowDebugSwitch()
{
	// F3でビヘイビアツリーエディターの表示をON/OFF
	static bool wasF3DownLastFrame = false;
	bool f3Down = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
	if (f3Down && !wasF3DownLastFrame) {
		showbtEditor = !showbtEditor;
	}
	wasF3DownLastFrame = f3Down;

	// F8で敵のパラメーターの表示をON/OFF
	static bool wasF8DownLastFrame = false;
	bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
	if (f8Down && !wasF8DownLastFrame) {
		showEnemyParameter = !showEnemyParameter;
	}
	wasF8DownLastFrame = f8Down;
}