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
#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/NodeBase.h" 

#include <imgui.h>


// =========================================================================
//  ライフサイクル・初期化
// =========================================================================

// デストラクタ
Enemy::~Enemy()
{
}

// 初期化
void Enemy::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	enemy = std::make_shared<Model>(device, "Data/Model/Player/Player/SK_SwordsmanGirl_02.gltf");

	// アニメーター生成
	animator = std::make_unique<Animator>();
	animator->Initialize(enemy);

	// 敵モデル・コライダー設定
	weight = 0.5f;
	radius = 0.8f;
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
	upperBodyNodeName = "spine_01";

	// ビヘイビアツリー初期化
	AISwitch = false;

	// モデルからアニメーション名一覧を取得してエディターに渡す
	std::vector<std::string> animNames;
	for (auto& anim : enemy->GetAnimations())
	{
		animNames.push_back(anim.name);
	}
	moveEditor.SetAnimationList(animNames);
	attackEditor.SetAnimationList(animNames);

	const std::string moveBTPath = "Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree.json";
	const std::string attackBTPath = "Data/Json/Enemy/BehaviorTreeEditor/BehaviorTree_Attack.json";

	// アニメ設定は2つ目を clearBefore=false にしないと、1つ目の分が消える
	LoadAnimationData(moveBTPath, true);
	LoadAnimationData(attackBTPath, false);

	moveGraph.Load(moveBTPath);
	moveEditor.ApplyToTree(moveBT.GetTree(), this, moveGraph);
	moveEditor.SetJsonPath(moveBTPath);

	attackGraph.Load(attackBTPath);
	attackEditor.ApplyToTree(attackBT.GetTree(), this, attackGraph);
	attackEditor.SetJsonPath(attackBTPath);

	// 状態のアニメーションを読み込む
	LoadAnimationData("Data/Json/Enemy/AnimationState/EnemyStates.json", false);

	// シーケンサーの初期化
	std::string seqPath = "Data/Json/Enemy/AnimationSequencer/EnemyAnimationSequence.json";
	sequencerEditor.SetJsonPath(seqPath);
	sequencerEditor.SetWindowName("Enemy Sequencer");
	animSequence.Load(seqPath);

	ChangeAnimationState("Idle");

	normalMagic.Initialize();
	normalMagicConfig.Load(); // 通常魔法の設定の読み込み
	rapidMagic.Initialize();
	rapidMagicConfig.Load(); // 連射魔法の設定の読み込み
	defenseMagic.Initialize(this); // 防御魔法の設定の読み込み

	lockOn.Initialize(this, &Player::Instance());
}


// =========================================================================
//  毎フレームのメイン処理・描画
// =========================================================================

// 更新処理
void Enemy::Update(float elapsedTime)
{
	// ダウン状態か
	bool isForced = UpdateForcedState(elapsedTime);

	// プレビュー中はAIを止める
	if (!isPreviewMode && AISwitch && !isForced)
	{
		moveBT.Update(elapsedTime);

		if (IsFullBodyAction())
		{
			attackBT.Abort(); // 撃っている途中なら後始末をして止める
		}
		else
		{
			attackBT.Update(elapsedTime);
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

	// タイマー更新
	if (attackCoolTimer > 0.0f)
	{
		attackCoolTimer -= elapsedTime;
	}

	// ステータス更新
	UpdateStatus(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	// シーケンサーの更新
	animSequence.Update(currentState, GetCurrentAnimationSeconds());
	SetBaseSpeed(animSequence.GetSpeed());

	// サウンド再生
	UpdateSounds(this->currentState);

	// 魔法更新処理
	magicManager.Update(elapsedTime);
	defenseMagic.Update(this, elapsedTime);
	debugShot(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	enemy->UpdateTransform(transform);
}

// 描画処理
void Enemy::Render(RenderContext& rc, ModelRenderer* renderer)
{
	if (showEnemy)
	{
		renderer->Draw(ShaderId::PBR, enemy);
	}
}

// GUI描画
void Enemy::DrawGUI()
{
	// 実行中のAIのステート名
	std::string moveStr = moveBT.GetActiveNode() ? moveBT.GetActiveNode()->GetName() : "";
	std::string attackStr = attackBT.GetActiveNode() ? attackBT.GetActiveNode()->GetName() : "";

	// 敵のパラメーター表示
	if (showEnemyParameter)
	{
		ImGui::Begin(u8"エネミー");
		{
			ImGui::Checkbox(u8"敵の表示", &showEnemy);

			float distans = GetDistanceToPlayer();
			ImGui::Text(u8"プレイヤーとの距離: %f", distans);

			// パラメーター全回復ボタン
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
			ImGui::Separator();

			// テスト
			if (ImGui::Button(u8"通常攻撃魔法"))
			{
				onNormalMagic = true;
			}
			else
			{
				onNormalMagic = false;
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"チャージ魔法"))
			{
				onChargeMagic = !onChargeMagic;
			}
			else
			{
				onChargeMagic = false;
			}
			ImGui::SameLine();
			ImGui::Checkbox(u8"連射魔法", &onRapidMagic);
			ImGui::Separator();
			if (ImGui::Checkbox(u8"防御魔法展開", &status.isGuarding))
			{
				defenseMagic.SetIsActive(status.isGuarding);
			}
			ImGui::Separator();



			// 体力
			ImGui::Text(u8"体力 : %f.0", status.currentHp);
			ImGui::DragFloat(u8"体力", &status.currentHp, 0.1f);
			ImGui::Separator();

			// 魔力
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

			// ヒート値
			ImGui::Text(u8"ヒート値 : %f.0", status.currentHeat);
			ImGui::DragFloat(u8"ヒート値", &status.currentHeat, 0.1f);
			ImGui::DragFloat(u8"ヒート値増加量", &status.heatIncreaseSpeed, 0.1f);
			ImGui::DragFloat(u8"ヒート値減少量", &status.heatRecoverSpeed, 0.1f);
			ImGui::Checkbox(u8"連射魔法テスト", &status.isRapidFire);
			ImGui::Separator();

			// 防御魔法
			ImGui::Text(u8"防御魔法の耐久値 : %f.0", status.currentDefenseDurability);
			ImGui::DragFloat(u8"防御魔法の魔力消費量", &status.defenseMagicCost, 0.1f);
			if (ImGui::Button(u8"耐久値減少"))
			{
				OnGuardHit(10.0f);
			}
			ImGui::Separator();

			// クールダウン・ステート情報
			ImGui::Text(u8"魔法クールダウン: %f", magicCoolDownTimer);
			ImGui::Separator();

			ImGui::Text(u8"移動ステート %s", moveStr.c_str());
			ImGui::Text(u8"攻撃ステート %s", attackStr.c_str());
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
				ImGui::DragFloat3("Angle", &a.x);

				angle.x = DirectX::XMConvertToRadians(a.x);
				angle.y = DirectX::XMConvertToRadians(a.y);
				angle.z = DirectX::XMConvertToRadians(a.z);

				ImGui::DragFloat3("Scale", &scale.x);
			}

			// 当たり判定情報
			if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("Radius:", &radius, 0.1f);
				ImGui::DragFloat("Height:", &height, 0.1f);
				ImGui::DragFloat("Collision Transform Offset:", &debugOffset, 0.1f);
			}
		}
		ImGui::End();
	}

	// BTEditorの表示/非表示
	if (showbtEditor)
	{
		ImGui::Begin("BTEditor");
		{
			if (ImGui::Button("AI:ON/OFF"))
			{
				AISwitch = !AISwitch;
			}

			if (ImGui::BeginTabBar("BTTabs"))
			{
				if (ImGui::BeginTabItem(u8"移動"))
				{
					moveEditor.Draw(moveGraph, moveBT, this);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem(u8"攻撃"))
				{
					attackEditor.Draw(attackGraph, attackBT, this);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::End();

		// シーケンサーを開くリクエストはどちらのエディターからも来る
		for (BehaviorTreeEditor* editor : { &moveEditor, &attackEditor })
		{
			if (editor->requestOpenSequencer)
			{
				isPreviewMode = true;
				editingState = editor->selectedAnimName;
				ChangeAnimationState(editingState);
				ImGui::SetWindowFocus("Enemy Sequencer");
				editor->requestOpenSequencer = false;
			}
		}

		// プレビューモード用の操作ウィンドウ
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
				isBlending = false;
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

// デバッグプリミティブ描画
void Enemy::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	// 敵の当たり判定カプセル
	{
		DirectX::XMFLOAT4X4 capsuleTransform;
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + debugOffset, position.z);
		DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

		renderer->DrawCapsule(capsuleTransform, radius, height, { 1, 1, 0, 1 });
	}

	// 防御魔法の当たり判定
	if (defenseMagic.GetIsActive())
	{
		renderer->DrawSphere(defenseMagic.GetDefensePosition(), defenseMagic.GetDefenseRadius(), { 0, 0, 1, 1 });
	}

	// 魔法の当たり判定描画
	magicManager.RenderDebugPrimitive(renderer);

	// 迎撃範囲の球
	if (status.isIntercept)
	{
		DirectX::XMFLOAT3 centerPos = {
			position.x + interceptCenterOffset.x,
			position.y + interceptCenterOffset.y,
			position.z + interceptCenterOffset.z
		};

		renderer->DrawSphere(centerPos, interceptRadius, { 0.0f, 1.0f, 1.0f, 0.5f });
	}
}


// =========================================================================
//  外部アクション・AI制御
// =========================================================================

// プレイヤーとの距離を取得
float Enemy::GetDistanceToPlayer() const
{
	Player& player = Player::Instance();

	DirectX::XMVECTOR ePos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR pPos = DirectX::XMLoadFloat3(&player.GetPosition());

	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(pPos, ePos);
	DirectX::XMVECTOR Length = DirectX::XMVector3Length(Vec);

	float distance = 0.0f;
	DirectX::XMStoreFloat(&distance, Length);

	return distance;
}

// ターゲットの座標に向かって旋回する
void Enemy::LockAtTarget(float elapsedTime, const DirectX::XMFLOAT3& targetPos)
{
	DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR PPos = DirectX::XMLoadFloat3(&targetPos);
	DirectX::XMVECTOR Dir = DirectX::XMVectorSubtract(PPos, EPos);

	// 水面でのみ向きを計算
	Dir = DirectX::XMVectorSetY(Dir, 0.0f);
	Dir = DirectX::XMVector3Normalize(Dir);

	DirectX::XMFLOAT3 lookDir;
	DirectX::XMStoreFloat3(&lookDir, Dir);

	Turn(elapsedTime, lookDir.x, lookDir.z, turnSpeed);
}

// ターゲットの方向を取得する
DirectX::XMFLOAT3 Enemy::GetMoveDirection(const DirectX::XMFLOAT3& movePos) const
{
	DirectX::XMVECTOR EPos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR PPos = DirectX::XMLoadFloat3(&movePos);

	DirectX::XMVECTOR Dir = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(PPos, EPos));

	DirectX::XMFLOAT3 direction;
	DirectX::XMStoreFloat3(&direction, Dir);

	return direction;
}

// =========================================================================
//  親子関係のオーバーライド
// =========================================================================

// アニメーションステート変化時のコールバック
void Enemy::OnStateChanged(const std::string& oldState, const std::string& newState)
{
}


// =========================================================================
//  内部ヘルパー
// =========================================================================

// サウンド再生の更新
void Enemy::UpdateSounds(const std::string& state)
{
	for (auto& data : animSequence.GetAnimations())
	{
		if (data.name != state) continue;

		for (auto& e : data.events)
		{
			if (e.type != EventType::Sound) continue;

			// 今フレームで発火したかをチェック
			if (animSequence.GetEvent(e.name))
			{
				AudioSource* se = GetOrLoadSound(e.soundName);
				if (se)
				{
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
	if (soundName.empty()) return nullptr;

	if (sounds.count(soundName))
	{
		return sounds[soundName].get();
	}

	std::string fileName = soundName;
	if (fileName.find(".wav") == std::string::npos)
	{
		fileName += ".wav";
	}
	std::string path = "Data/Sound/" + fileName;

	auto source = Audio::Instance().LoadAudioSource(path.c_str());

	if (source)
	{
		sounds[soundName] = std::unique_ptr<AudioSource>(source);
		return sounds[soundName].get();
	}
	else
	{
		sounds[soundName] = nullptr;
		return nullptr;
	}
}

bool Enemy::IsFullBodyAction()
{
	return status.isAvoid ;
}


bool Enemy::UpdateForcedState(float elapsedTime)
{
	switch (forcedState)
	{
	case Enemy::ForcedState::None:
	{
		// 立ち上がりの瞬間だけ拾う
		bool burstStarted = status.isBurstTrigger && !wasBurstTrigger;
		bool breakStarted = status.isBreak && !wasBreak;
		wasBurstTrigger = status.isBurstTrigger;
		wasBreak = status.isBreak;

		if (status.isDown)
		{
			moveBT.Abort(); // この間はAIを動かさない
			ChangeAnimationState("DownLoop");
			forcedState = ForcedState::Down;
			return true;
		}

		if (burstStarted || breakStarted)
		{
			moveBT.Abort();
			attackBT.Abort();
			ChangeAnimationState(burstStarted ? "Burst" : "Break");
			forcedState = burstStarted ? ForcedState::Burst : ForcedState::Break;
			return true;
		}
		return false;
	}
	case Enemy::ForcedState::Down:
		// 魔力が満タンになったら起き上がる
		if (!status.isDown)
		{
			ChangeAnimationState("DownEnd");
			forcedState = ForcedState::GetUp;
		}
		return true;

	case Enemy::ForcedState::GetUp:
		// アニメーションが終わったらAIを元に戻す
		if (GetCurrentAnimationSeconds() >= GetCurrentAnimationLength())
		{
			forcedState = ForcedState::None;
			return false;
		}
		return true;

	case ForcedState::Burst:
	case ForcedState::Break:
		if (status.isDown) { forcedState = ForcedState::None; return true; } // ダウンを優先させる（次のフレームで None からダウンに入る）
		if (GetCurrentAnimationSeconds() >= GetCurrentAnimationLength())
		{
			forcedState = ForcedState::None;
			return false;
		}
		return true;
	}
	return false;
}

// GUI表示切り替えキー入力
void Enemy::ShowDebugSwitch()
{
	// F3でビヘイビアツリーエディターの表示をON/OFF
	static bool wasF3DownLastFrame = false;
	bool f3Down = (GetAsyncKeyState(VK_F3) & 0x8000) != 0;
	if (f3Down && !wasF3DownLastFrame)
	{
		showbtEditor = !showbtEditor;
	}
	wasF3DownLastFrame = f3Down;

	// F8で敵パラメーターの表示をON/OFF
	static bool wasF8DownLastFrame = false;
	bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
	if (f8Down && !wasF8DownLastFrame)
	{
		showEnemyParameter = !showEnemyParameter;
	}
	wasF8DownLastFrame = f8Down;
}

// 魔法を撃つデバック
void Enemy::debugShot(float elapsedTime)
{
	Player& player = Player::Instance();

	// 魔法を撃つ方向を計算する
	DirectX::XMVECTOR enemyPos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&player.GetPosition());
	DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(playerPos, enemyPos);
	DirectX::XMFLOAT3 dir;
	DirectX::XMStoreFloat3(&dir, DirectX::XMVector3Normalize(diff));

	// 通常攻撃魔法
	if(onNormalMagic)
	{

		MagicSpawnInfo spawnInfo;
		spawnInfo.position = position;
		spawnInfo.direction = dir;
		spawnInfo.owner = this;
		spawnInfo.target = &player;
		spawnInfo.holdTime = 3.0f;

		normalMagic.Emit(&magicManager, spawnInfo, normalMagicConfig);
		magicCoolDownTimer = coolDownValue;
		normalMagic.ResetCharge();
	}
	// チャージ魔法
	else if (onChargeMagic)
	{
		MagicSpawnInfo spawnInfo;
		spawnInfo.position = position;
		spawnInfo.direction = dir;
		spawnInfo.owner = this;
		spawnInfo.target = &player;
		spawnInfo.holdTime = 3.0f;

		normalMagic.Emit(&magicManager, spawnInfo, normalMagicConfig);
		magicCoolDownTimer = fullcChargeCoolDownValue;
		normalMagic.ResetCharge();
	}

	// 連射魔法
	if (onRapidMagic)
	{
		status.isRapidFire = true;

		MagicSpawnInfo spawnInfo;
		spawnInfo.position = position;
		spawnInfo.direction = dir;
		spawnInfo.owner = this;
		spawnInfo.target = &player;

		spawnInfo.holdTime = 0.05f;

		rapidMagicConfig.damage = 0.01f;
		rapidMagicConfig.gaurdDamage = 2.0f;
		rapidMagicConfig.hitRadius = 0.25f;
		rapidMagicConfig.speed = 150.0f;

		// 連射魔法を呼び出す
		rapidMagic.Update(elapsedTime, &magicManager, spawnInfo, rapidMagicConfig, status.isRapidFire);
	}
	else
	{
		status.isRapidFire = false;
	}
}
