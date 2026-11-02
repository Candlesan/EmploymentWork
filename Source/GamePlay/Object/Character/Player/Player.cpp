#include "Player.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"
#include "System/Audio/Audio.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/CameraShake/CameraShake.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h" 
#include "GamePlay/Object/Stage/Stage.h"

#include "imgui_node_editor.h"
#include <imgui.h>


// =========================================================================
//  ライフサイクル・初期化
// =========================================================================

// 初期化
void Player::Initialize()
{
	// デバイスを取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	player = std::make_shared<Model>(device, "Data/Model/Player/Player/SK_SwordsmanGirl_02.gltf");
	weapon.model = std::make_shared<Model>(device, "Data/Model/Weapon/Player/Weapon/wizard_staff.gltf");
	weapon.scale.x = weapon.scale.y = weapon.scale.z = 0.01f; //　武器の大きさを調整

	// アニメーターを生成
	animator = std::make_unique<Animator>();
	animator->Initialize(player);

	// パラメータ初期化
	moveSpeed = 7.0f;
	maxMoveSpeed = 20.0f;
	status.currentHp = 100.0f;
	status.maxHp = 100.0f;
	status.currentMagic = 100.0f;
	status.maxMagic = 100.0f;
	status.maxDefenseDurability = 100.0f;
	status.currentDefenseDurability = 100.0f;

	// 当たり判定の設定
	weight = 0.5f;
	radius = 0.8f;
	height = 1.0f;
	CollisionOffset = 0.8;

	// 杖の位置設定
	weapon.position = { 0.07f, 0.0f, 0.03f };
	weapon.angle = { -0.06, 0.23f, -1.13f };

	// ボーン名の初期化
	rootMotionNodeName = "root";
	upperBodyNodeName = "spine_01";

	// 各種システムの初期化（
	InitializeSequencer();          // シーケンサーの初期化
	InitializeAnimationNodeEditor(); // ノードエディターの初期化

	normalMagic.Initialize();
	normalMagicConfig.Load(); // 通常魔法の設定の読み込み
	rapidMagic.Initialize();
	rapidMagicConfig.Load(); // 連射魔法の設定の読み込み
	defenseMagic.Initialize(this); // 防御魔法の設定の読み込み
	CustomEffectManager::Instance().LoadEffectConfig("Barreir", "Data/Json/Effect/Barreir.json"); // 防御魔法のエフェクトの読み込み

	// アニメーションポーズと初期ステートの設定
	player->GetNodePoses(nodePoses);
	player->GetNodePoses(oldNodePoses);
	SetupInitialAnimationState();    // 初期アニメーションの設定

	InitializeHeadDirection(); // 頭の向きを初期化
	CustomEffectManager::Instance().LoadEffectConfig("Barrier", "Data/Json/Effect/Barrier.json");
	CustomEffectManager::Instance().LoadEffectConfig("Intercept", "Data/Json/Effect/Intercept.json");
	CustomEffectManager::Instance().GetOrCreateResident("Intercept");
}

// シーケンサーの初期化
void Player::InitializeSequencer()
{
	// シーケンサーの初期化
	std::string path = "Data/Json/Player/AnimationSequencer/PlayerAnimationSequence.json";
	sequencerEditor.SetJsonPath(path);
	sequencerEditor.SetWindowName("Player Sequencer");
	animSequence.Load(path);
}

// ノードエディタの初期化
void Player::InitializeAnimationNodeEditor()
{
	{
		namespace fs = std::filesystem;
		std::string folderPath = "Data/Json/Player/AnimationNodeEditor/";

		// ディレクトリが存在するかをチェック
		if (fs::exists(folderPath) && fs::is_directory(folderPath))
		{
			//ディレクトリ内のJsonを一つずつ取り出す
			for (const auto& entry : fs::directory_iterator(folderPath))
			{
				// 拡張子が.jsonかを調べる
				if (entry.is_regular_file() && entry.path().extension() == ".json")
				{
					// フルパスを取得する（タブの名前にするため）
					std::string filePath = entry.path().string();

					// 新しいグラフをvectorに追加してLoadする
					AnimationTransitionGraph newGraph;
					newGraph.Load(filePath);

					// ファイル名をグラフ名にする
					newGraph.graphName = entry.path().stem().string();

					transitionGraphs.push_back(newGraph);

					LoadAnimationData(filePath);
				}
			}
		}

		if (transitionGraphs.empty())
		{
			AnimationTransitionGraph defaultGraph;
			defaultGraph.graphName = "BaseState";

			// ファイルがない場合の初期ノード作成
			defaultGraph.AddNode("Idle", { 100, 100 });

			if (!fs::exists(folderPath))
			{
				fs::create_directories(folderPath);
			}

			// 最後に保存
			defaultGraph.Save("Data/Json/Player/AnimationNodeEditor/BaseState.json");
			transitionGraphs.push_back(defaultGraph);
			LoadAnimationData("Data/Json/Player/AnimationNodeEditor/BaseState.json");
		}
	}
}

// 頭の向きを初期化
void Player::InitializeHeadDirection()
{
	// 頭ノード取得
	int headNodeIndex = player->GetNodeIndex("neck_01");
	Model::Node& headNode = player->GetNodes().at(headNodeIndex);

	// 初期姿勢時の頭のノードのローカル空間前方向を求める
	DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&headNode.worldTransform);
	DirectX::XMMATRIX InverseWorldTransform = DirectX::XMMatrixInverse(nullptr, WorldTransform);
	DirectX::XMVECTOR HeadWorldForward = DirectX::XMVectorSet(0.0f, 0.0f, 10.0f, 0.0f);
	DirectX::XMVECTOR HeadLocalForward = DirectX::XMVector3TransformNormal(HeadWorldForward, InverseWorldTransform);

	HeadLocalForward = DirectX::XMVector3Normalize(HeadLocalForward);
	DirectX::XMStoreFloat3(&headLocalForward, HeadLocalForward);

}

// アニメーションのセットアップ処理
void Player::SetupInitialAnimationState()
{
	// ゲーム開始時は、1つ目のグラフをスタックの１番上に詰む！
	if (!transitionGraphs.empty())
	{
		activeGraphStack.push_back(0); // BaseState

		int targetIndex = -1;
		for (int i = 0; i < (int)transitionGraphs.size(); i++)
		{
			if (transitionGraphs[i].graphName == "State_Idle")
			{
				targetIndex = i;
				break;
			}
		}

		if (targetIndex != -1)
		{
			activeGraphStack.push_back(targetIndex);

			std::string firstAnim = "";
			for (auto& link : transitionGraphs[targetIndex].links)
			{
				if (link.transition.fromState == "Entry")
				{
					firstAnim = link.transition.toState;
					break;
				}
			}
			if (firstAnim != "") ChangeAnimationState(firstAnim);
		}
		else
		{
			ChangeAnimationState("Idle");
		}
	}
}

// 終了化
void Player::Finalize()
{
	sounds.clear();
}


// =========================================================================
//  毎フレームのメイン処理・描画
// =========================================================================

// 更新処理
void Player::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	if (gamePad.GetButtonDown() & GamePad::BTN_START)
	{
		status.isDead = false;
		status.currentHp = status.maxHp;
	}

	// 今、文字入力中かどうかを取得
	bool isTyping = ImGui::GetIO().WantCaptureKeyboard;

	// GUIの表示/非表示
	ShowDebugSwitch();

	// タイマーを更新する関数
	UpdateTimers(elapsedTime);

	// ステータスの更新
	UpdateStatus(elapsedTime);

	// プレイヤーの状態を計算する関数
	CalculateStatusParameters();

	// 魔法を撃つ
	MagicInput(elapsedTime);

	InputLockOn();
	Camera& camera = CameraManager::Instance().GetMainCamera();
	lockOn.Update(elapsedTime, camera.GetFront());

#ifdef _DEBUG
	if (!isTyping && !isPreviewMode)
	{
		// 移動処理
		InputMove(elapsedTime);

		// 飛行更新
		UpdateFly(elapsedTime);

		// アニメーション遷移
		UpdateStateTransitions(elapsedTime);

		// 迎撃の構え：0.348秒で止める
		if (currentOverlayState == "AS_Combo_Attack_04_01_Seq_0"
			&& animator->GetUpperCurrentSeconds() >= interceptStanceStopSeconds && !status.isAvoid)
		{
			animator->SetUpperSpeed(0.0f);
		}
		else
		{
			animator->SetUpperSpeed(1.0f);
		}

		// 上半身のアニメーション遷移
		UpdateUpperBodyTransitions(elapsedTime);
	}
	else if (isTyping && !isPreviewMode)
	{
		// もし普通にゲームしている最中に文字入力を始めたら、足踏みしないようにIdleに戻す
		ChangeAnimationState("State_Idle");
	}
#endif // _DEBUG

#ifdef NDEBUG
	// 移動処理
	InputMove(elapsedTime);

	// 飛行更新
	UpdateFly(elapsedTime);

	// アニメーション遷移
	UpdateStateTransitions(elapsedTime);

	// 迎撃の構え：0.348秒で止める
	if (currentOverlayState == "AS_Combo_Attack_04_01_Seq_0"
		&& animator->GetUpperCurrentSeconds() >= interceptStanceStopSeconds && !status.isAvoid)
	{
		animator->SetUpperSpeed(0.0f);
	}

	// 上半身のアニメーション遷移
	UpdateUpperBodyTransitions(elapsedTime);
#endif // NDEBUG



	// プレビューモード用の強制ループ処理
	if (isPreviewMode)
	{
		if (GetCurrentAnimationSeconds() >= GetCurrentAnimationLength())
		{
			animationSeconds = 0.0f; // ループさせる
		}
	}

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	bank(elapsedTime);

	// 魔法更新処理
	magicManager.Update(elapsedTime);
	defenseMagic.Update(this, elapsedTime);

	// シーケンサーとアニメーションの再生を同期させる
	std::string targetSequenceName = isPreviewMode ? editingState : currentState;
	animSequence.Update(targetSequenceName, GetCurrentAnimationSeconds());
	SetBaseSpeed(animSequence.GetSpeed());

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	UpdateTransform();
	player->UpdateTransform(transform);

	// サウンド再生
	UpdateSounds(this->currentState);

	// 移動時の背骨の回転角度を求める関数
	CalculateSpineRotation(elapsedTime);

	// ルックアット処理更新
	UpdateLockAt(elapsedTime);

	// 腕のIK
	ArmIK(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	player->UpdateTransform(transform);

	// 武器のアタッチメント処理
	UpdateWeaponAttachment("hand_r");
}

// 描画処理
void Player::Render(RenderContext& rc, ModelRenderer* renderer)
{
	if (showPlayer)
	{
		renderer->Draw(ShaderId::PBR, player);
		renderer->Draw(ShaderId::PBR, weapon.model);
	}

	// 魔法描画
	magicManager.Render(rc);
}

// GUI描画
void Player::DrawGUI()
{
	// プレイヤーのパラメーターのON/OFF
	if (showPlayerParameter)
	{
		ImGui::Begin(u8"プレイヤー");
		{
			ImGui::Checkbox(u8"プレイヤーの表示", &showPlayer);

			// アニメーション遷移状態
			if (ImGui::CollapsingHeader(u8"アニメーションステート状況", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto it = stateConfigs.find(currentState);

				if (it != stateConfigs.end())
				{
					ImGui::Text(u8"現在の全身ステート: %s", it->second.animationName.c_str());
				}
				else
				{
					ImGui::Text(u8"現在の全身ステート: なし(または不明)");
				}

				ImGui::Text(u8"現在の上半身ステート: %s", currentOverlayState.c_str());
			}

			// トランスフォーム情報
			if (ImGui::CollapsingHeader(u8"トランスフォーム (位置・回転・拡縮)", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3(u8"座標 (Position)", &position.x);

				DirectX::XMFLOAT3 a;
				a.x = DirectX::XMConvertToDegrees(angle.x);
				a.y = DirectX::XMConvertToDegrees(angle.y);
				a.z = DirectX::XMConvertToDegrees(angle.z);
				ImGui::DragFloat3(u8"回転角度 (Angle)", &a.x);
				angle.x = DirectX::XMConvertToRadians(a.x);
				angle.y = DirectX::XMConvertToRadians(a.y);
				angle.z = DirectX::XMConvertToRadians(a.z);

				ImGui::DragFloat3(u8"拡縮比 (Scale)", &scale.x);
			}

			// パラメーター
			if (ImGui::CollapsingHeader(u8"パラメータ", ImGuiTreeNodeFlags_DefaultOpen))
			{
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
				ImGui::Text(u8"魔法クールダウン: %f", magicCoolDownTimer);
				ImGui::DragFloat(u8"魔力", &status.currentMagic, 0.1f);
				ImGui::DragFloat(u8"基本の魔力回復速度", &status.magicRecoverSpeed, 0.1f);
				ImGui::DragFloat(u8"魔力の回復する遅延", &magicRecoverDelay, 0.1f);
				ImGui::Separator();

				ImGui::Text(u8"防御魔法の耐久値 : %f.0", status.currentDefenseDurability);
				ImGui::DragFloat(u8"防御魔法の魔力消費量", &status.defenseMagicCost, 0.1f);
				if (GetIsGaurd()) {
					ImGui::Text(u8"防衛状態: ガード中");
				}
				else {
					ImGui::Text(u8"防衛状態: ガードなし");
				}
				ImGui::Separator();

				ImGui::Text(u8"首の回転: %f.0", debugAngle);
				ImGui::Separator();

				ImGui::Text(u8"基本移動速度: %f.0", moveSpeed);
				ImGui::Text(u8"物理速力 (Velocity): %.2f, %.2f, %.2f", velocity.x, velocity.y, velocity.z);
				ImGui::Text(u8"スティック入力強度: %.3f", moveLength);
				ImGui::Text(u8"ターゲットとの距離: %.3f", enemyFromDistance);
				if (IsGround())
				{
					ImGui::Text(u8"接地状態: 地面");
				}
				else
				{
					ImGui::Text(u8"接地状態: 空中");
				}
				ImGui::Separator();

				// 入力方向のデバッグ表示
				const char* debugDirNames[] = {
					u8"入力なし", u8"前", u8"右前", u8"左前", u8"右", u8"後", u8"右後", u8"左後", u8"左"
				};
				ImGui::Text(u8"入力角度: %.2f", debug_degree);
				ImGui::Text(u8"判定された入力方向: %s", debugDirNames[debug_dirIndex]);

				ImGui::Separator();

				ImGui::DragFloat(u8"スティック補間速度", &lerpSpeed, 0.1f);
				ImGui::Text(u8"滑らかな入力強度: %f.1", smoothMoveLength);

				if (status.isAvoid) {
					ImGui::Text(u8"アクション状態: 回避中");
				}
				else {
					ImGui::Text(u8"アクション状態: 通常時");
				}

				ImGui::DragFloat(u8"地上切り替えし速度", &turnaroundRate, 0.1f);
				ImGui::DragFloat(u8"空中切り替えし速度", &airTurnaroundRate, 0.1f);
				ImGui::DragFloat(u8"滑り度合い", &overSpeedDecel, 0.1f);
				ImGui::DragFloat(u8"空中の摩擦係数", &airFrictionRate, 0.1f);
			}

			lockOn.DrawGUI();
		}

		// 魔法関係
		if (ImGui::CollapsingHeader(u8"通常魔法"))
		{
			if (ImGui::Button(u8"保存##Normal"))
			{
				normalMagicConfig.Save();
			}
			ImGui::SameLine();
			if (ImGui::Button(u8"読み込み##Normal"))
			{
				normalMagicConfig.Load(); // JSONから読み込み
			}
			ImGui::Separator();

			ImGui::Text(u8"(通常撃ち / チャージ切り替え時間: %.2f秒)", normalMagicConfig.chargeTimeThreshold);
			ImGui::DragFloat(u8"切り替え閾値時間", &normalMagicConfig.chargeTimeThreshold, 0.05f, 0.1f, 3.0f);
			ImGui::DragFloat(u8"射程距離 (Range)", &normalMagicConfig.range, 1.0f, 10.0f, 300.0f);
			ImGui::Separator();

			ImGui::Text(u8"--- 通常撃ち ---");
			ImGui::DragFloat(u8"通常ダメージ", &normalMagicConfig.normalDamage, 1.0f);
			ImGui::DragFloat(u8"通常判定の太さ (半径)", &normalMagicConfig.normalRadius, 0.1f, 0.1f, 5.0f);
			ImGui::DragFloat(u8"通常の魔力消費量", &normalMagicConfig.normalMagicCost, 0.1f);
			ImGui::Separator();

			ImGui::Text(u8"--- チャージ撃ち  ---");
			ImGui::DragFloat(u8"チャージダメージ", &normalMagicConfig.chargeDamage, 1.0f);
			ImGui::DragFloat(u8"チャージ判定の太さ (半径)", &normalMagicConfig.chargeRadius, 0.1f, 0.1f, 10.0f);
			ImGui::DragFloat(u8"チャージの魔力消費量", &normalMagicConfig.chargeMagicCost, 0.1f);
			ImGui::DragFloat(u8"フルチャージの維持中の魔力消費量", &normalMagicConfig.chargeHoldMagicCost, 0.1f);
			ImGui::Separator();

			ImGui::DragFloat3(u8"手元オフセット##normal", &normalMagicConfig.spawnOffset.x, 0.1f);
			ImGui::Separator();

			ImGui::DragFloat(u8"クールダウン", &coolDownValue, 0.1f);
			ImGui::DragFloat(u8"フルチャージのクールダウン", &fullcChargeCoolDownValue, 0.1f);
			ImGui::Separator();

			normalMagic.DrawGUI(normalMagicConfig);
		}

		if (ImGui::CollapsingHeader(u8"連射魔法"))
		{
			rapidMagic.DrawGUI(rapidMagicConfig);
		}

		//　防御魔法
		defenseMagic.DrawGUI();

		if (ImGui::CollapsingHeader(u8"迎撃"))
		{
			ImGui::DragFloat(u8"迎撃をする距離", &interceptRadius, 0.1f);
		}

		// 当たり判定情報
		if (ImGui::CollapsingHeader(u8"キャラクター当たり判定 (カプセル)"))
		{
			ImGui::DragFloat(u8"判定の半径", &radius, 0.1f);
			ImGui::DragFloat(u8"判定の高さ", &height, 0.1f);
			ImGui::DragFloat(u8"デバッグ用描画オフセット", &CollisionOffset, 0.1f);
		}

		// 武器のアタッチメント情報
		if (ImGui::CollapsingHeader(u8"杖アタッチメント"))
		{
			ImGui::DragFloat3(u8"位置##wand", &weapon.position.x, 0.01f);
			ImGui::DragFloat3(u8"角度##wand", &weapon.angle.x, 0.01f);
			ImGui::DragFloat3(u8"スケール##wand", &weapon.scale.x, 0.01f);
		}

		// 移動時の背骨の傾き
		if (ImGui::CollapsingHeader(u8"背骨のしなり (バンキング設定)"))
		{
			ImGui::DragFloat(u8"しなりの強さ", &targetSpineAngle, 0.1f);
			ImGui::DragFloat(u8"しなりの戻り速度", &spineAngleSpeed, 0.1f);
		}

		// 移動時の背骨の傾き
		if (ImGui::CollapsingHeader(u8"飛行中の傾き"))
		{
			ImGui::Text(u8"X軸 : %f.0", flightBankX);
			ImGui::Text(u8"Z軸 : %f.0", flightBankZ);
			ImGui::DragFloat(u8"傾き速度", &bankSpeed, 0.1f);
		}
		ImGui::End();
	}

	// アニメーション遷移エディターのON/OFF
	if (showAnimarionTransitionEditor)
	{
		ImGui::Begin(u8"ステート遷移グラフエディタ");
		std::string clickedNode = transitionEditor.Draw(
			transitionGraphs,
			currentState, activeGraphStack,
			currentOverlayState, upperGraphStack
		);
		ImGui::End();

		// ノードがダブルクリックされたらプレビューモードに切り替える
		if (clickedNode != "")
		{
			editingState = clickedNode;
			AnimNode* node = FindNodeByName(editingState);
			if (node)
			{
				// 本当のアニメーション名（WindSpear_B）を使ってモデルのアニメーションを再生！
				std::string realAnimName = node->assetName.empty() ? node->config.animationName : node->assetName;
				ChangeAnimationState(realAnimName);
			}
			isPreviewMode = true;
			ImGui::SetWindowFocus("Player Sequencer");
		}

		if (isPreviewMode)
		{
			ImGui::Begin(u8"プレビュー再生タイムライン");

			if (ImGui::Button(u8"▶ ゲーム画面に戻る"))
			{
				isPreviewMode = false;
				ChangeAnimationState("Idle");
			}
			ImGui::SameLine();

			float length = GetCurrentAnimationLength();

			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[%s]", editingState.c_str());
			ImGui::SameLine();
			ImGui::Text(u8"  時間: %.2f / %.2f 秒", animationSeconds, length);

			ImGui::SetNextItemWidth(-1);
			if (ImGui::SliderFloat("##animTime", &animationSeconds, 0.0f, length))
			{
				isBlending = false;
			}

			ImGui::End();
		}

		std::string displayState = isPreviewMode ? editingState : currentState;

		auto* animData = animSequence.FindAnimationData(displayState);
		if (animData)
		{
			AnimNode* currentNode = FindNodeByName(displayState);
			if (currentNode && !currentNode->assetName.empty())
			{
				animData->assetName = currentNode->assetName;
			}
		}

		sequencerEditor.DrawEditor(
			animSequence,
			displayState,
			GetCurrentAnimationSeconds(),
			GetCurrentAnimationLength()
		);

		RefreshStateConfigs();
	}
}

// デバックプリミティブ描画
void Player::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// プレイヤーの当たり判定
	DirectX::XMFLOAT4X4 capsuleTransform;
	if (!status.isAvoid)
	{
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + CollisionOffset, position.z);
		DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

		renderer->DrawCapsule(capsuleTransform, radius, height, { 0, 1, 0, 1 });
	}

	// 魔法の当たり判定描画
	magicManager.RenderDebugPrimitive(renderer);

	if (defenseMagic.GetIsActive())
	{
		renderer->DrawSphere(defenseMagic.GetDefensePosition(), defenseMagic.GetDefenseRadius(), { 0, 0, 1, 1 });
	}

	DirectX::XMFLOAT3 centerPos = {
		position.x + interceptCenterOffset.x,
		position.y + interceptCenterOffset.y,
		position.z + interceptCenterOffset.z
	};

	// 迎撃範囲の球
	if (status.isIntercept)
	{
		renderer->DrawSphere(centerPos, interceptRadius, { 0.0f, 1.0f, 1.0f, 0.5f });
	}

	if (lockOn.IsLocked())
	{
		DirectX::XMFLOAT3 predicted = lockOn.GetPredictedPosition(position, rapidMagicConfig.speed);
		renderer->DrawSphere(predicted, 1.0f, { 1.0f, 1.0f, 0.0f, 1.0f });
	}

	renderer->DrawSphere(GetAimPoint(100.0f), 0.5f, {1, 0, 0, 1});
}


// =========================================================================
//  外部アクション命令
// =========================================================================

// 魔法射出
void Player::MagicInput(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	MagicBase* targetMagic = nullptr;
	auto& effectManager = CustomEffectManager::Instance();

	// 連射魔法用にカメラの前方向を取得する
	Camera& camera = CameraManager::Instance().GetMainCamera();
	DirectX::XMFLOAT3 defultDir = camera.GetFront();
	DirectX::XMFLOAT3 defultPos = camera.GetEye();
	DirectX::XMVECTOR cameraPos = DirectX::XMLoadFloat3(&defultPos);
	DirectX::XMVECTOR cameraFront = DirectX::XMLoadFloat3(&defultDir);
	DirectX::XMVECTOR Target = DirectX::XMVectorAdd(cameraPos, DirectX::XMVectorScale(cameraFront, rapidMagicConfig.speed * rapidMagicConfig.lifeTimer));
	DirectX::XMStoreFloat3(&defultTarget, Target);

	// LTを押したら
	status.isIntercept = (gamePad.GetButton() & GamePad::BTN_LEFT_TRIGGER) != 0;
	status.isRapidFire = (gamePad.GetButton() & GamePad::BTN_RIGHT_TRIGGER) != 0;
	bool isHold = (rbButtonHoldTime > RB_THRESHOLD); // RB長押しでチャージ（消える可能性あり）
	
	// 通常魔法系更新
	normalMagic.Update(elapsedTime, this, normalMagicConfig, isHold);

	// 通常魔法の発射位置を計算する
	muzzlePos = {
		position.x + normalMagicConfig.spawnOffset.x,
		position.y + normalMagicConfig.spawnOffset.y,
		position.z + normalMagicConfig.spawnOffset.z,
	};

	// ボタンを離したら予約をする
	if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER && CanControl())
	{
		isShot = true;
	}

	// チャージ魔法 (今後通所魔法と置き換えになるか消えるかもしれやん)
	if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER && CanControl())
	{
		if (normalMagic.IsFullCharged())
		{
			// フルチャージ発射（もう一度押した瞬間）
			MagicSpawnInfo spawnInfo;
			spawnInfo.position = position;
			spawnInfo.direction = (targetMagic != nullptr) ? interceptDir : lockOn.GetAimDirection(muzzlePos, 100000.0f);// 即着弾やから数値をデカくしておく
			spawnInfo.owner = this;
			spawnInfo.target = enemy;
			spawnInfo.targetPos = lockOn.IsLocked() ? lockOn.GetPredictedPosition(muzzlePos, 100000.0f) : defultTarget;
			spawnInfo.holdTime = rbButtonHoldTime;
			spawnInfo.angleY = angle.y;

			normalMagic.Emit(&magicManager, spawnInfo, normalMagicConfig);
			magicCoolDownTimer = fullcChargeCoolDownValue;
			normalMagic.ResetCharge();
		}
	}

	// 通常攻撃魔法
	if (animSequence.GetEvent("shot") && isShot && magicCoolDownTimer <= 0.0f)
	{
		DirectX::XMFLOAT3 aimPos = GetAimPoint(normalMagicConfig.range + 10.0f);

		DirectX::XMVECTOR Muzzle = DirectX::XMLoadFloat3(&muzzlePos);
		DirectX::XMVECTOR Aim = DirectX::XMLoadFloat3(&aimPos);
		DirectX::XMFLOAT3 aimDir;
		DirectX::XMStoreFloat3(&aimDir, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(Aim, Muzzle)));

		MagicSpawnInfo spawnInfo;
		spawnInfo.position = position;
		spawnInfo.direction = (targetMagic != nullptr) ? interceptDir
			: lockOn.GetAuotAim() ? lockOn.GetAimDirection(muzzlePos, 100000.0f) : aimDir;
		spawnInfo.owner = this;
		spawnInfo.target = enemy;
		spawnInfo.targetPos = lockOn.IsLocked() ? lockOn.GetPredictedPosition(muzzlePos, 100000.0f) : aimPos;
		spawnInfo.holdTime = rbButtonHoldTime;
		spawnInfo.angleY = angle.y;

		normalMagic.Emit(&magicManager, spawnInfo, normalMagicConfig);
		CameraShakeManager::Instance().Add(0.05f, 0.15f); // 位置なし

		magicCoolDownTimer = coolDownValue;
		normalMagic.ResetCharge();
		isShot = false;
	}

	// 迎撃
	if (status.isIntercept && CanControl())
	{
		if(flightController.IsFlying())
		PlayActionGraph("Intercept");

		// 迎撃の中心位置
		DirectX::XMFLOAT3 centerPos = {
			position.x + interceptCenterOffset.x,
			position.y + interceptCenterOffset.y,
			position.z + interceptCenterOffset.z
		};

		// 一番近い魔法を取得する
		MagicManager& manager = enemy->GetMagicManager();
		auto targets = manager.FindNearProjectiles(this, centerPos, interceptRadius);
		if (!targets.empty())
		{
			targetMagic = targets[0];
		}

		if (targetMagic)
		{
			// 迎撃する方向を計算する
			DirectX::XMVECTOR magicPos = DirectX::XMLoadFloat3(&targetMagic->GetPosition());
			DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&position);
			DirectX::XMStoreFloat3(&interceptDir, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(magicPos, playerPos)));

			// 撃つ前に見た目を迎撃用にする
			rapidMagic.SetEffect("InterceptShot");
			rapidMagic.SetHitEffect("Intercept");

			// 魔法の設定
			MagicSpawnInfo spawnInfo;
			spawnInfo.position = position;
			spawnInfo.direction = interceptDir;
			spawnInfo.owner = this;
			spawnInfo.target = enemy;
			spawnInfo.targetPos = targetMagic->GetPosition();

			rapidMagicConfig.damage = 0.0f;
			rapidMagicConfig.gaurdDamage = 0.0f;
			rapidMagicConfig.hitRadius = 0.25f;
			rapidMagicConfig.speed = 80.0f;

			rapidMagic.Update(elapsedTime, &magicManager, spawnInfo, rapidMagicConfig, true);

			// 敵が連射魔法なら撃ち落とす
			if (targetMagic->GetMagicKind() == MagicKind::Rapid && targetMagic->GetIsAlive())
			{
				RecoverMagic(targetMagic->GetMagicmagicRefund());
				CustomEffectManager::Instance().PlayBurst("Intercept", targetMagic->GetPosition(), { 0, 0, 0 }, { 1, 1, 1 });
				CameraShakeManager::Instance().Add(targetMagic->GetPosition(), 10.0f, 0.5f, 0.1f);
				targetMagic->OnTerminate();
			}
		}
	}
	// 連射魔法
	else if (status.isRapidFire && CanControl())
	{
		rapidMagic.SetEffect("RapidMagic");
		rapidMagic.SetHitEffect("RapidHit");
		// 魔法の設定
		MagicSpawnInfo spawnInfo;
		spawnInfo.position = position;
		spawnInfo.direction = (targetMagic != nullptr) ? interceptDir : lockOn.GetAimDirection(position, rapidMagicConfig.speed);
		spawnInfo.owner = this;
		spawnInfo.target = enemy;
		spawnInfo.targetPos = lockOn.IsLocked() ? lockOn.GetPredictedPosition(position, rapidMagicConfig.speed) : defultTarget;
		spawnInfo.holdTime = rtButtonHoldTime;

		rapidMagicConfig.damage = 0.01f;
		rapidMagicConfig.gaurdDamage = 2.0f;
		rapidMagicConfig.hitRadius = 0.25f;
		rapidMagicConfig.speed = 150.0f;

		// 連射魔法を呼び出す
		rapidMagic.Update(elapsedTime, &magicManager, spawnInfo, rapidMagicConfig, status.isRapidFire);
	}

	// 防御魔法
	if ((gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)  && !status.isBreak && CanControl()) // ダウン状態でもバースト状態でもないとき
	{
		SetIsGaurd(true);

		// まだ生きているかを確認する
		if (!effectManager.Exists(barrierEffect))
		{
			DirectX::XMFLOAT3 effectAngle = {
				DirectX::XMConvertToDegrees(angle.x),
				DirectX::XMConvertToDegrees(angle.y),
				DirectX::XMConvertToDegrees(angle.z)
			};
			barrierEffect = CustomEffectManager::Instance().PlayEffect("Barreir", position, effectAngle, { 1.5f, 1.5f, 1.5f });
		}
		else
		{
			// 出してる間はプレイヤーに追従させる
			effectManager.SetPosition(barrierEffect, defenseMagic.GetDefensePosition());
		}
	}
	else
	{
		SetIsGaurd(false);

		// ボタンを離したら止めて、ポインタも空に戻す
		if (barrierEffect)
		{
			effectManager.Stop(barrierEffect);
			barrierEffect = InvalidEffect;
		}
	}
}

// グラフを追加する
void Player::AddGraph(std::string name)
{
	AnimationTransitionGraph newGraph;

	// デフォルトのタブ名をセットする
	newGraph.InitializeAsNew(name);

	// 初期ノードをセット
	newGraph.AddNode("Idle", { 100, 100 });

	// リストに追加する
	transitionGraphs.push_back(newGraph);

	// Jsonファイルとして保存する
	std::string path = "Data/Json/Player/AnimationNodeEditor/" + name + ".json";
	transitionGraphs.back().Save(path);
}

// サウンドを流す
void Player::UpdateSounds(const std::string& state)
{
	auto& animations = animSequence.GetAnimations();
	for (auto& data : animations)
	{
		if (data.name != state) continue;

		for (auto& e : data.events)
		{
			AudioSource* se = GetOrLoadSound(e.soundName);
			if (se)
			{
				se->Play(false);
			}
		}
	}
}

void Player::InputLockOn()
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 右スティック押し込みでハードロック切り替え
	if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_THUMB)
	{
		if (lockOn.IsHardLock())
		{
			lockOn.SetHardLock(false);  // 解除
		}
		else if (lockOn.IsLocked())
		{
			lockOn.SetHardLock(true);   // ソフトロック中の相手に固定
		}
	}
}

void Player::UpdateFly(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// ダウン状態になると落下するようにする
	if (!CanControl())
	{
		flightController.SetFlightMode(FlightMode::OFF);
		flightController.Update(this, elapsedTime);
		return;
	}

	// 入力をまとめる
	FlightInput input;
	input.isAButtonDown = (gamePad.GetButton() & GamePad::BTN_A) != 0;
	input.isAButtonUp = (gamePad.GetButtonUp() & GamePad::BTN_A) != 0;

	// 入力状態を渡す
	flightController.ProcessInput(input, elapsedTime);

	// 状態を更新する
	flightController.Update(this, elapsedTime);
}

// =========================================================================
//  セッター（インライン以外）
// =========================================================================

// =========================================================================
//  ゲッター（インライン以外）
// =========================================================================

//　武器の位置を取得
DirectX::XMFLOAT3 Player::GetWeaponPosition() const
{
	DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

	// スケールを除去（描画側と同じ処理）
	DirectX::XMVECTOR scale, rot, pos;
	DirectX::XMMatrixDecompose(&scale, &rot, &pos, weaponWorld);
	DirectX::XMMATRIX baseMatrix =
		DirectX::XMMatrixRotationQuaternion(rot) *
		DirectX::XMMatrixTranslationFromVector(pos);

	// 描画側と完全に同じ行列を作る
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

// 武器の向きを取得
DirectX::XMFLOAT3 Player::GetWeaponDirection() const
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


// =========================================================================
//  親子関係のオーバーライド
// =========================================================================

// 死んだときに呼ばれる奴
void Player::OnDead()
{
	status.isDead = true; // 死亡フラグを立てる
}


// =========================================================================
//  内部ヘルパー（移動・入力）
// =========================================================================

// スティック入力値から移動ベクトルを取得
DirectX::XMFLOAT3 Player::GetMoveVec() const
{
	// 入力情報を取得
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisLX();
	float ay = gamePad.GetAxisLY();

	// カメラ方向とスティックの入力値によって進行方向を計算する
	Camera& camera = CameraManager::Instance().GetMainCamera();
	const DirectX::XMFLOAT3& cameraRight = camera.GetRight();
	const DirectX::XMFLOAT3& cameraFront = camera.GetFront();

	// 移動ベクトルはXZ平面に水平なベクトルになるようにする

	// カメラ右方向ベクトルをXZ単位ベクトルに変換
	float cameraRightX = cameraRight.x;
	float cameraRightZ = cameraRight.z;
	float cameraRightLength = sqrtf(cameraRightX * cameraRightX + cameraRightZ * cameraRightZ);
	if (cameraRightLength > 0.0f)
	{
		// 単位ベクトル化
		cameraRightX /= cameraRightLength;
		cameraRightZ /= cameraRightLength;
	}

	// カメラ前方向ベクトルをXZ単位ベクトルに変換
	float cameraFrontX = cameraFront.x;
	float cameraFrontZ = cameraFront.z;
	float cameraFrontLength = sqrtf(cameraFrontX * cameraFrontX + cameraFrontZ * cameraFrontZ);
	if (cameraFrontLength > 0.0f)
	{
		// 単位ベクトル化
		cameraFrontX /= cameraFrontLength;
		cameraFrontZ /= cameraFrontLength;
	}

	// スティックの水平入力値をカメラ右方向に反映し、
	// スティックの垂直入力値をカメラ前方向に反映し、
	// 進行ベクトルを計算する
	DirectX::XMFLOAT3 vec;
	vec.x = (cameraRightX * ax) + (cameraFrontX * ay);
	vec.z = (cameraRightZ * ax) + (cameraFrontZ * ay);
	//Y軸方向には移動しない
	vec.y = 0.0f;

	return vec;
}

// 入力処理
void Player::InputMove(float elapsedTime)
{
	if (!CanControl()) return;

	GamePad& gamePad = Input::Instance().GetGamePad();
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	bool isAimDodge = lockOn.IsHardLock() || status.isIntercept;

	if ((currentState == "AS_Combo_Attack_04_02_Seq_0" || currentState == "AS_Combo_Attack_03_04_Seq_0"))
	{
		// 魔法中は移動速度を0にする
		velocity = { 0, 0, 0 };
	}
	if (currentState == "AS_Dodge_Combat_to_Run_Fast_F_0_Seq_0" ||
		(isAimDodge && currentState.rfind("AS_Dodge_Combat_to_Run_", 0) == 0))
	{
		// 回避中は
		Move(moveVec.x, moveVec.z, moveSpeed * 1.0f);
	}
	else if (useRootMotion || useRootMotionEx && lockOn.IsHardLock())
	{
		// ルートモーションを使ってる時の移動速度
		Move(moveVec.x, moveVec.z, moveSpeed * 0.7f);
	}
	else if (useRootMotion || useRootMotionEx)
	{
		// ルートモーションを使ってる時の移動速度
		Move(moveVec.x, moveVec.z, 0.0f);
	}
	else if (flightController.IsFlying() && normalMagic.IsFullCharged())
	{
		// 飛行中かつチャージ済み移動速度
		Move(moveVec.x, moveVec.z, fullChargeSpeed);
	}
	else if (flightController.IsFlying())
	{
		// 飛行中の移動速度
		Move(moveVec.x, moveVec.z, flightSpeed);
	}
	else
	{
		// 通常の移動速度
		Move(moveVec.x, moveVec.z, moveSpeed);
	}

	DirectX::XMFLOAT3 aimPos = lockOn.IsLocked()
		? lockOn.GetPredictedPosition(muzzlePos, 100000.0f)
		: defultTarget;

	// 回転処理
	if (currentState == "AS_Combo_Attack_04_02_Seq_0" || currentState == "AS_Combo_Attack_03_04_Seq_0")
	{
		// 魔法中は敵の方を向く
		TurnToPosition(elapsedTime, aimPos, turnSpeed);
	}
	else if ((status.isIntercept || status.isRapidFire) && flightController.IsFlying())
	{
		// 空中で迎撃・連射している時だけ敵の方を向く
		TurnToPosition(elapsedTime, enemy->GetPosition(), turnSpeed);
	}
	else
	{
		Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
	}
}

// 入力されたスティックから相対的な移動方向を計算する関数
RelativeInputDir Player::CalculateRelativeInputDir()
{
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);

	if (moveLength > 0.01f) // ある程度入力がある場合のみ判定
	{
		// キャラクターの前・右ベクトル
		float ForwardX = sinf(angle.y), ForwardZ = cosf(angle.y);
		float RightX = cosf(angle.y), RightZ = -sinf(angle.y);

		// 入力ベクトルとの内積
		float dotForward = moveVec.x * ForwardX + moveVec.z * ForwardZ;
		float dotRight = moveVec.x * RightX + moveVec.z * RightZ;

		// 相対角度を求める
		float relativeAngle = atan2f(dotRight, dotForward);
		float degree = DirectX::XMConvertToDegrees(relativeAngle);
		if (degree < 0) degree += 360.0; // 0 ～ 360度に補正

		// 角度を8等分して前後左右を判定する
		if (degree >= 337.5 || degree < 22.5f)
		{
			return RelativeInputDir::Forward;
		}
		else if (degree >= 22.5f && degree < 67.5f)
		{
			return RelativeInputDir::ForwardRight;
		}
		else if (degree >= 67.5f && degree < 112.5f)
		{
			return RelativeInputDir::Right;
		}
		else if (degree > 112.5f && degree < 157.5f)
		{
			return RelativeInputDir::BackwardRight;
		}
		else if (degree >= 157.5f && degree < 202.5f)
		{
			return RelativeInputDir::Backward;
		}
		else if (degree >= 202.5f && degree < 247.5f)
		{
			return RelativeInputDir::BackwardLeft;
		}
		else if (degree >= 247.5f && degree < 292.5f)
		{
			return RelativeInputDir::Left;
		}
		else if (degree >= 292.5f && degree < 337.5)
		{
			return RelativeInputDir::ForwardLeft;
		}

		debug_degree = degree;
	}
	else
	{
		return RelativeInputDir::None;
	}
}


// =========================================================================
//  内部ヘルパー（ステートマシン）
// =========================================================================

// 状態遷移更新処理
void Player::UpdateStateTransitions(float elapsedTime)
{
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 条件設定
	TransitionContext ctx = InputTransitionContext(elapsedTime);

	if (activeGraphStack.empty()) return;

	int currentIndex = activeGraphStack.back();
	AnimationTransitionGraph* currentGraph = &transitionGraphs[currentIndex];
	int targetLevel = (int)activeGraphStack.size() - 1;
	std::string evalStateForTrans = currentState;
	std::string nextState = currentState;

	// 親の割り込み処理
	for (int i = (int)activeGraphStack.size() - 1; i >= 0; i--)
	{
		int gIdx = activeGraphStack[i];
		AnimationTransitionGraph* g = &transitionGraphs[gIdx];

		// この階層における「現在のステート名」を決める
		// 自分が一番上（子）なら実際の currentState。
		// 親グラフを評価する時は、自分の1つ上に乗っている「子グラフの箱の名前」が現在位置になる。
		std::string evalState = currentState;
		if (i < (int)activeGraphStack.size() - 1)
		{
			evalState = transitionGraphs[activeGraphStack[i + 1]].graphName; // 現在地から１個上
		}

		// 遷移できるか評価する
		std::string result = g->EvaluateTransitions(evalState, ctx);

		// 遷移先が見つかったら即遷移
		if (result != evalState)
		{
			nextState = result;
			currentGraph = g;
			targetLevel = i;
			evalStateForTrans = evalState;
			break;
		}
	}

	// どの階層でも遷移が出来なかったら終了
	if (currentGraph == nullptr) return;

	// 遷移する時に指定された階層までスタックを下す
	while ((int)activeGraphStack.size() - 1 > targetLevel)
	{
		activeGraphStack.pop_back();
	}

	// 今のステートから変化があった場合のみ処理
	if (nextState != currentState)
	{
		// 遷移したらExit直後のフラグを戻す
		isSubGraphExited = false;

		// アクションを実行する
		const AnimationTransition* trans = currentGraph->GetTransition(evalStateForTrans, nextState);
		if (trans)
		{
			// グラフで評価して遷移する
			for (auto& action : trans->actions)
			{
				switch (action.type)
				{
				case TransitionActionType::MoveSpeed: moveSpeed = action.value; break;
				case TransitionActionType::TurnSpeed: turnSpeed = DirectX::XMConvertToRadians(action.value); break;
				case TransitionActionType::SetIsAvoid: status.isAvoid = (action.value != 0.0f); break;
				}
			}
		}


		//脱出（Exit）処理
		// 階層ノードかどうかを調べる前に、まずはExitかどうか判定するのが正解
		if (nextState == "Exit")
		{
			// 現在のステート名を子グラフの名前（=親グラフでの箱の名前）に上書きする
			currentState = currentGraph->graphName;

			// スタックから現在の子グラフを下ろして、親グラフに戻る
			activeGraphStack.pop_back();

			bufferedButton = 0;

			// 親グラフではアニメーション終了扱いにして、すぐ次へ遷移できるようにする
			isSubGraphExited = true;

			return;
		}


		// 遷移の結果、その行き先のノードが「階層ノード(SubGraph)」かどうかを調べる
		AnimNode* nextNode = nullptr;
		for (auto& n : currentGraph->nodes)
		{
			if (n.StateName == nextState) { nextNode = &n; break; }
		}

		// 潜り込み処理
		if (nextNode && nextNode->type == NodeType::SubGraph)
		{
			EnterSubGraph(nextNode->subGraphPath, ctx);
		}
		else if (nextNode && nextNode->type == NodeType::Mix)
		{
			// 逆探知関数でAとBのアニメーション名を取ってくる
			std::string stateA = currentGraph->GetInputNodeName(nextState, InputSlot::A);
			std::string stateB = currentGraph->GetInputNodeName(nextState, InputSlot::B);

			// 名前からアニメーション番号に変換する
			int animIndexA = GetModel()->GetAnimationIndex(stateConfigs[stateA].animationName.c_str());
			int animIndexB = GetModel()->GetAnimationIndex(stateConfigs[stateB].animationName.c_str());

			// Animatorに加算ブレンドを依頼する
			animator->PlayBaseMix(animIndexA, animIndexB, nextNode->mixRate, false, 0.2f, false, false);

			// ステート名をMixノードに変える
			previousState = currentState;
			currentState = nextState;
			bufferedButton = 0;
		}
		else
		{
			// 次に遷移するノードの設定を持ってくる
			auto it = stateConfigs.find(nextState);
			if (it != stateConfigs.end())
			{
				const AnimationConfig& nextConfig = it->second;

				// ノードに設定されたボーン名があるかどうかで、アニメーターのレイヤーを分ける
				if (!nextConfig.targetBoneName.empty())
				{
					// ボーン名が空では無ければマスクをかける
					StartOverlayAnimation(nextState);
				}
				else
				{
					// ボーン名が空だったら全身アニメーションにする
					ChangeAnimationState(nextState);
				}
			}
			bufferedButton = 0;
		}
	}
}

// 上半身の状態遷移更新処理
void Player::UpdateUpperBodyTransitions(float elapsedTime)
{
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 条件設定
	TransitionContext ctx = InputTransitionContext(elapsedTime);
	ctx.animSeconds = animator->GetUpperCurrentSeconds();
	ctx.animLength = animator->GetUpperAnimationLength();

	if (upperGraphStack.empty()) return;

	int currentIndex = upperGraphStack.back();
	AnimationTransitionGraph* currentGraph = &transitionGraphs[currentIndex];
	int targetLevel = (int)upperGraphStack.size() - 1;
	std::string evalStateForTrans = currentOverlayState;
	std::string nextState = currentOverlayState;

	// 親の割り込み処理
	for (int i = (int)upperGraphStack.size() - 1; i >= 0; i--)
	{
		int gIdx = upperGraphStack[i];
		AnimationTransitionGraph* g = &transitionGraphs[gIdx];

		// この階層における「現在のステート名」を決める
		// 自分が一番上（子）なら実際の currentOverlayState。
		// 親グラフを評価する時は、自分の1つ上に乗っている「子グラフの箱の名前」が現在位置になる。
		std::string evalState = currentOverlayState;
		if (i < (int)upperGraphStack.size() - 1)
		{
			evalState = transitionGraphs[upperGraphStack[i + 1]].graphName; // 現在地から１個上
		}

		// 遷移できるか評価する
		std::string result = g->EvaluateTransitions(evalState, ctx);

		// 遷移先が見つかったら即遷移
		if (result != evalState)
		{
			nextState = result;
			currentGraph = g;
			targetLevel = i;
			evalStateForTrans = evalState;
			break;
		}
	}

	// どの階層でも遷移が出来なかったら終了
	if (currentGraph == nullptr) return;

	// 遷移する時に指定された階層までスタックを下す
	while ((int)upperGraphStack.size() - 1 > targetLevel)
	{
		upperGraphStack.pop_back();
	}

	// 今のステートから変化があった場合のみ処理
	if (nextState != currentOverlayState)
	{
		// アクションを実行する
		const AnimationTransition* trans = currentGraph->GetTransition(evalStateForTrans, nextState);
		if (trans)
		{
			// グラフで評価して遷移する
			for (auto& action : trans->actions)
			{
				switch (action.type)
				{
				case TransitionActionType::MoveSpeed: moveSpeed = action.value; break;
				case TransitionActionType::TurnSpeed: turnSpeed = DirectX::XMConvertToRadians(action.value); break;
				case TransitionActionType::SetIsAvoid: status.isAvoid = (action.value != 0.0f); break;
				}
			}
		}


		//脱出（Exit）処理
		// 階層ノードかどうかを調べる前に、まずはExitかどうか判定するのが正解
		if (nextState == "Exit")
		{
			// 上半身を止める
			animator->StopUpper();

			if (upperGraphStack.size() > 1)
			{
				// 親グラフがいるなら、そこに戻る
				currentOverlayState = currentGraph->graphName;
				upperGraphStack.pop_back();
			}
			else
			{
				// 親がいない（これが一番下）なら、popせず待機状態に戻すだけ
				currentOverlayState = "Entry";
			}

			// 名前を空にする
			currentOverlayState = "";

			// スタックから現在の子グラフを下ろして、親グラフに戻る
			upperGraphStack.pop_back();

			return;
		}


		// 遷移の結果、その行き先のノードが「階層ノード(SubGraph)」かどうかを調べる
		AnimNode* nextNode = nullptr;
		for (auto& n : currentGraph->nodes)
		{
			if (n.StateName == nextState) { nextNode = &n; break; }
		}

		// 潜り込み処理
		if (nextNode && nextNode->type == NodeType::SubGraph)
		{
			AnimationTransitionGraph* subGraph = nullptr;
			int targetIndex = -1;

			// パスからファイル名(グラフ名)を取り出す
			char filename[MAX_PATH];
			_splitpath_s(nextNode->subGraphPath.c_str(), nullptr, 0, nullptr, 0, filename, MAX_PATH, nullptr, 0);
			std::string targetName = filename;


			// Initializeで既に全ロードされているグラフ群から探す
			for (int i = 0; i < (int)transitionGraphs.size(); i++)
			{
				if (transitionGraphs[i].graphName == targetName)
				{
					subGraph = &transitionGraphs[i];
					targetIndex = i;
					break;
				}
			}

			if (subGraph && targetIndex != -1)
			{
				// スタックに子グラフを積んで潜る！
				upperGraphStack.push_back(targetIndex);

				// 「Entry」ノードからの矢印を探して、最初のアニメーションを決める
				std::string firstState = subGraph->EvaluateTransitions("Entry", ctx);
				if (firstState != "Entry" && firstState != "")
				{
					const AnimationTransition* trans = subGraph->GetTransition("Entry", firstState);
					if (trans)
					{
						for (auto& action : trans->actions)
						{
							switch (action.type)
							{
							case TransitionActionType::MoveSpeed: moveSpeed = action.value; break;
							case TransitionActionType::TurnSpeed: turnSpeed = DirectX::XMConvertToRadians(action.value); break;
							case TransitionActionType::SetIsAvoid: status.isAvoid = (action.value != 0.0f); break;
							}
						}
					}
					// 最初のステートへ切り替え！
					if (firstState != "")
					{
						StartOverlayAnimation(firstState);
						bufferedButton = 0; // サブグラフへ潜り込んだ時のリセット
					}
				}
				else
				{
					// 万が一 Entry からの条件がどれも満たされなかったら、適当なノードを最初にする（元のフォールバック）
					firstState = "";
					for (auto& n : subGraph->nodes)
					{
						if (n.type == NodeType::Animation && n.StateName != "Entry" && n.StateName != "Exit")
						{
							firstState = n.StateName;
							break;
						}
					}
				}

				// 最初のステートへ切り替え！
				if (firstState != "") StartOverlayAnimation(firstState);
			}
		}
		else if (nextNode && nextNode->type == NodeType::Mix)
		{
			// 逆探知してAとBの名前を取得！
			std::string stateA = currentGraph->GetInputNodeName(nextState, InputSlot::A);
			std::string stateB = currentGraph->GetInputNodeName(nextState, InputSlot::B);

			// 名前からアニメーション番号に変換する
			int animIndexA = GetModel()->GetAnimationIndex(stateConfigs[stateA].animationName.c_str());
			int animIndexB = GetModel()->GetAnimationIndex(stateConfigs[stateB].animationName.c_str());

			// Animator工場に上半身用のMix発注！
			animator->PlayUpperMix(animIndexA, animIndexB, nextNode->mixRate, false, stateConfigs[stateA].targetBoneName, 0.2f);

			// ステート名を更新
			currentOverlayState = nextState;
			bufferedButton = 0;
		}
		else
		{
			// 次に遷移するノードの設定を持ってくる
			auto it = stateConfigs.find(nextState);
			if (it != stateConfigs.end())
			{
				const AnimationConfig& nextConfig = it->second;

				// ノードに設定されたボーン名があるかどうかで、アニメーターのレイヤーを分ける
				if (!nextConfig.targetBoneName.empty())
				{
					// ボーン名が空では無ければマスクをかける
					StartOverlayAnimation(nextState);
				}
				else
				{
					// ボーン名が空だったら全身アニメーションにする
					ChangeAnimationState(nextState);
				}
			}
			bufferedButton = 0;
		}
	}
}

// 条件設定
TransitionContext Player::InputTransitionContext(float elapsedTime)
{
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
	GamePad& gamePad = Input::Instance().GetGamePad();

	smoothMoveLength += (moveLength - smoothMoveLength) * lerpSpeed * elapsedTime;

	// 先行入力処理
	if (animSequence.GetRange("InputBuffer"))
	{
		// 既に記憶しているボタンに今フレームでおされたボタンを追加
		bufferedButton |= gamePad.GetButtonDown();
	}

	// RBボタンの長押し判定
	if (gamePad.GetButton() & GamePad::BTN_RIGHT_SHOULDER)
	{
		rbButtonHoldTime += elapsedTime;
	}
	else
	{
		rbButtonHoldTime = 0.0f;
	}

	// RTボタンの長押し判定
	if (gamePad.GetButton() & GamePad::BTN_RIGHT_TRIGGER)
	{
		rtButtonHoldTime += elapsedTime;
	}
	else
	{
		rtButtonHoldTime = 0.0f;
	}

	UpdateStateBehavior();

	TransitionContext ctx;
	ctx.position = GetPosition();
	ctx.animSeconds = GetCurrentAnimationSeconds();
	ctx.animLength = GetCurrentAnimationLength();

	// 子グラフからExitした直後はアニメーション終了とみなす
	if (isSubGraphExited)
	{
		ctx.animSeconds = ctx.animLength;
	}

	ctx.moveLength = moveLength;
	ctx.enemyFromdistance = enemyFromDistance;
	ctx.buttonDown = gamePad.GetButtonDown() | bufferedButton;
	ctx.buttonHeld = gamePad.GetButton();
	ctx.HoldTime = rbButtonHoldTime;
	ctx.buttonUp = gamePad.GetButtonUp();
	ctx.IsGround = IsGround();
	ctx.isLockOn = lockOn.IsHardLock();
	ctx.isGuarding = GetIsGaurd();
	ctx.IsFalling = falling;
	ctx.IsCancelable = animSequence.GetRange("Cancelable");
	ctx.IsDown = status.isDown;
	ctx.inputDir = CalculateRelativeInputDir();
	ctx.IsDead = IsDead();
	ctx.currentHP = status.currentHp;
	ctx.IsAiming = status.isIntercept;

	// 移動速度を計算する
	const DirectX::XMFLOAT3 v = GetVelocity();
	ctx.MoveSpeed = sqrtf(v.x * v.x + v.z * v.z);

	return ctx;
}

// 潜り込み処理
void Player::EnterSubGraph(const std::string& subGraphPath, const TransitionContext& ctx) {
	// パスからファイル名(グラフ名)を取り出す
	char filename[MAX_PATH];
	_splitpath_s(subGraphPath.c_str(), nullptr, 0, nullptr, 0, filename, MAX_PATH, nullptr, 0);
	std::string targetName = filename;


	// Initializeで既に全ロードされているグラフ群から探す
	AnimationTransitionGraph* subGraph = nullptr;
	int targetIndex = -1;
	for (int i = 0; i < (int)transitionGraphs.size(); i++)
	{
		if (transitionGraphs[i].graphName == targetName)
		{
			subGraph = &transitionGraphs[i];
			targetIndex = i;
			break;
		}
	}

	if (subGraph && targetIndex != -1)
	{
		activeGraphStack.push_back(targetIndex);

		// 「Entry」ノードからの矢印を探して、最初のアニメーションを決める
		std::string firstState = subGraph->EvaluateTransitions("Entry", ctx);

		if (firstState != "Entry" && !firstState.empty())
		{
			const AnimationTransition* trans = subGraph->GetTransition("Entry", firstState);
			if (trans)
			{
				for (auto& action : trans->actions)
				{
					switch (action.type)
					{
					case TransitionActionType::MoveSpeed: moveSpeed = action.value; break;
					case TransitionActionType::TurnSpeed: turnSpeed = DirectX::XMConvertToRadians(action.value); break;
					case TransitionActionType::SetIsAvoid: status.isAvoid = (action.value != 0.0f); break;
					}
				}
			}

			// 最初のステートへ切り替え！
			if (!firstState.empty())
			{
				// そのステート（ノード）の情報を取得する
				AnimNode* firstNode = nullptr;
				for (auto& n : subGraph->nodes)
				{
					if (n.StateName == firstState)
					{
						firstNode = &n;
						break;
					}
				}

				// もし見つかったノードが「さらにサブグラフ」だったら、もう一回この関数を呼んで深く潜る！
				if (firstNode && firstNode->type == NodeType::SubGraph)
				{
					EnterSubGraph(firstNode->subGraphPath, ctx);
				}
				else
				{
					// サブグラフじゃない（＝本物のアニメーションノードだ！）なら、ついに遷移を実行！
					ChangeAnimationState(firstState);
					bufferedButton = 0;
				}
			}
		}
		else
		{
			// 万が一 Entry からの条件がどれも満たされなかったら、適当なノードを最初にする（元のフォールバック）
			firstState = "";
			for (auto& n : subGraph->nodes)
			{
				if (n.type == NodeType::Animation && n.StateName != "Entry" && n.StateName != "Exit")
				{
					firstState = n.StateName;
					break;
				}
			}
		}

		// 最初のステートへ切り替え！
		if (firstState != "") ChangeAnimationState(firstState);
	}

}

// アクション（コンボ）のグラフを呼び出す関数
void Player::PlayActionGraph(const std::string& graphName)
{
	// 既に何かのコンボ中なら、新しくは上書きしない
	if (!upperGraphStack.empty()) return;

	// 指定されたグラフ（State_Magicなど）を探して、上半身スタックに積む！
	for (int i = 0; i < (int)transitionGraphs.size(); i++)
	{
		if (transitionGraphs[i].graphName == graphName)
		{
			upperGraphStack.push_back(i);
			currentOverlayState = "Entry"; // スタート地点にセット
			return;
		}
	}
}

// 遷移以外の細かい条件
void Player::UpdateStateBehavior()
{
}

// ノードの設定を最新にする関数
void Player::RefreshStateConfigs()
{
	// メモリ上の transitionGraphs から最新の config データを stateConfigs に反映する
	for (const auto& graph : transitionGraphs)
	{
		for (const auto& node : graph.nodes)
		{
			if (node.type == NodeType::Animation)
			{
				stateConfigs[node.StateName] = node.config;
			}
		}
	}
}



// =========================================================================
//  内部ヘルパー（物理・タイマー・その他）
// =========================================================================

// タイマー処理を更新する関数
void Player::UpdateTimers(float elapsedTime)
{
	// 魔法のクールダウン
	if (magicCoolDownTimer > 0.0f)
	{
		magicCoolDownTimer -= elapsedTime;
	}
}

// プレイヤーの状態を計算する関数
void Player::CalculateStatusParameters()
{
	if (status.isBreak && !wasBreak)
	{
		PlayActionGraph("Break");
	}
	wasBreak = status.isBreak;

	//GamePad& gamePad = Input::Instance().GetGamePad();
	//if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_THUMB)
	//{
	//	hardLock = !hardLock;
	//	lockOn.SetHardLock(hardLock);
	//}

	// 敵との距離の計算
	DirectX::XMVECTOR Ppos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR Epos = DirectX::XMLoadFloat3(&enemy->GetPosition());
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Epos, Ppos);
	enemyFromDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(Vec));
}

// ルックアット処理更新
void Player::UpdateLockAt(float elapsedTime)
{
	// ターゲット位置を計算する
	Camera& camera = CameraManager::Instance().GetMainCamera();
	DirectX::XMVECTOR camFront = DirectX::XMLoadFloat3(&camera.GetFront());
	DirectX::XMVECTOR playerPos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR targetVec = DirectX::XMVectorAdd(playerPos, DirectX::XMVectorScale(camFront, 70.0f));
	DirectX::XMFLOAT3 targetPosition;
	DirectX::XMStoreFloat3(&targetPosition, targetVec);

	// ターゲットがキャラクターの後ろに来た時に制限を掛ける
	float s = sinf(angle.y);
	float c = cosf(angle.y);
	DirectX::XMVECTOR playerFront = DirectX::XMVectorSet(s, 0.0f, c, 0.0f);

	// プレイヤーの前方向とカメラの前方向の内積を取る
	float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(playerFront, camFront));

	float targetWeight = 0.0f;
	bool isMagicEndingSoon = false;

	if (currentOverlayState == "Intercept")
	{
		float currentSec = animator->GetUpperCurrentSeconds();
		float lengthSec = animator->GetUpperAnimationLength();

		if (lengthSec - currentSec <= 0.15f)
		{
			isMagicEndingSoon = true;
		}
	}

	// 範囲内かつ、内積が前方を向いているとき
	if (((enemyFromDistance <= 10.0f && dot >= 0.0f) || currentOverlayState == "Intercept") && !isMagicEndingSoon)
	{
		targetWeight = 1.0f;
	}
	else
	{
		targetWeight = 0.0f;
	}

	// 線形補間をする
	lookAtWeight += (targetWeight - lookAtWeight) * 5.0f * elapsedTime;

	// もしウェイトが0になったら処理をスキップする
	if (lookAtWeight <= 0.001f && targetWeight == 0.0f)
	{
		lookAtWeight = 0.0f;
		return;
	}

	// 頭のノードを取得する
	int spineNodeIndex = player->GetNodeIndex("spine_02");
	int headNodeIndex = player->GetNodeIndex("neck_01");
	if (headNodeIndex < 0 || spineNodeIndex < 0) return;
	Model::Node& spineNode = player->GetNodes().at(spineNodeIndex);
	Model::Node& headNode = player->GetNodes().at(headNodeIndex);

	// 頭から見たターゲットへの方向ベクトルを求める
	DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&headNode.worldTransform);
	DirectX::XMMATRIX InverseWorldTransform = DirectX::XMMatrixInverse(nullptr, WorldTransform);

	// ターゲットまでのベクトルをローカル空間に変換
	DirectX::XMVECTOR HeadPos = DirectX::XMLoadFloat3(&position);
	HeadPos = DirectX::XMVectorSetY(HeadPos, position.y + 1.6f);
	DirectX::XMVECTOR TragetPos = DirectX::XMLoadFloat3(&targetPosition);
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(TragetPos, HeadPos);
	Vec = DirectX::XMVector3Normalize(Vec);

	// 2つのローカルベクトル
	DirectX::XMVECTOR LocalDirection = DirectX::XMVector3TransformNormal(Vec, InverseWorldTransform);
	DirectX::XMVECTOR LocalForward = DirectX::XMLoadFloat3(&headLocalForward);

	// ローカル空間で回転処理
	// 回転軸を求める
	DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(LocalForward, LocalDirection);

	// 回転角を求める
	float angle = acosf(DirectX::XMVectorGetX(DirectX::XMVector3Dot(LocalForward, LocalDirection)));
	debugAngle = angle;

	float maxLookAngle = DirectX::XMConvertToRadians(120.0f);
	if (angle > maxLookAngle)
	{
		angle = maxLookAngle;
	}

	DirectX::XMVECTOR currentSpineRotation = DirectX::XMLoadFloat4(&spineNode.rotation);
	DirectX::XMVECTOR SpineNewRot = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionRotationAxis(Axis, angle * 0.3f), currentSpineRotation);

	// 今のウェイトの割合にで滑らかにブレンドする
	DirectX::XMVECTOR FinalSpineRot = DirectX::XMQuaternionSlerp(
		currentSpineRotation,
		SpineNewRot,
		lookAtWeight);

	DirectX::XMStoreFloat4(&spineNode.rotation, FinalSpineRot);



	// クォータニオンを使ってボーンに掛ける
	DirectX::XMVECTOR CurrentHeadRotation = DirectX::XMLoadFloat4(&headNode.rotation);
	DirectX::XMVECTOR NewHeadRotation = DirectX::XMQuaternionMultiply(DirectX::XMQuaternionRotationAxis(Axis, angle * 0.7f), CurrentHeadRotation);

	// 今のウェイトの割合にで滑らかにブレンドする
	DirectX::XMVECTOR FinalHeadRot = DirectX::XMQuaternionSlerp(
		CurrentHeadRotation,
		NewHeadRotation,
		lookAtWeight);


	DirectX::XMStoreFloat4(&headNode.rotation, FinalHeadRot);
}

// 移動時の背骨の回転角度を求める関数
void Player::CalculateSpineRotation(float elapsedTime)
{
	// 今回の背骨の回転角度と前回の背骨の回転角度の差分を求める
	float deltaAngleY = angle.y - previousAngleY;

	// XMScalarModAngle : 角度を -π ～ +π の範囲に正規化する関数
	deltaAngleY = DirectX::XMScalarModAngle(deltaAngleY);

	// 目標角度を求める
	float targetBankAngle = deltaAngleY * targetSpineAngle * smoothMoveLength;

	// 曲がる最大値を設定する
	float maxLimit = DirectX::XMConvertToRadians(30.0f);

	//targetBankAngle を - maxLimit ～ + maxLimit の範囲にする
	targetBankAngle = std::clamp(targetBankAngle, -maxLimit, maxLimit);

	if (moveLength < 0.1f || !IsGround()) {
		targetBankAngle = 0.0f;
	}

	// 傾きを滑らかにする
	currentBankAngle += (targetBankAngle - currentBankAngle) * spineAngleSpeed * elapsedTime;
	previousAngleY = angle.y;

	if (animator)
	{
		animator->SetBankAngle(currentBankAngle);
	}
}

//　ノードの名前を探す関数
AnimNode* Player::FindNodeByName(const std::string& stateName)
{
	// Playerが持っているすべての遷移グラフ（transitionGraphs）から探す
	for (auto& graph : transitionGraphs)
	{
		for (auto& node : graph.nodes)
		{
			// 名前（StateName）が一致するノードが見つかったらそのポインタを返す
			if (node.StateName == stateName)
			{
				return &node;
			}
		}
	}
	return nullptr; // 見つからなかったら nullptr
}

// 指定の方向へ補間しながら向く
void Player::TurnToPosition(float elapsedTime, const DirectX::XMFLOAT3& target, float speed)
{
	float dx = target.x - position.x;
	float dz = target.z - position.z;
	if (dx * dx + dz * dz < 0.0001f) return; // 真上・真下にいる時は向きを変えない

	float targetAngleY = atan2f(dx, dz); // 敵の方向に即座に向く
	float diff = targetAngleY - angle.y;
	diff = DirectX::XMScalarModAngle(diff);
	float t = (std::min)(speed * elapsedTime, 1.0f); // 行き過ぎ防止
	angle.y += diff * t;
}

// 飛行中に身体を傾ける処理
void Player::bank(float elapsedTime)
{
	if (!flightController.IsFlying() || animSequence.GetRange("shot"))
	{
		// 跳んでいないときはもとに戻す
		flightBankX = 0.0f;
		flightBankZ = 0.0f;
		angle.x = 0.0f;
		angle.z = 0.0f;
		return;
	}

	// 移動ベクトルを取得
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	// プレイヤーの前方向・右方向を計算する
	float forwardX = sinf(angle.y);
	float forwardZ = cosf(angle.y);

	float rightX = cosf(angle.y);
	float rightZ = -sinf(angle.y);

	// プレイヤーから見た前後・左右の入力量
	float inputForward = moveVec.x * forwardX + moveVec.z * forwardZ;
	float inputRight = moveVec.x * rightX + moveVec.z * rightZ;

	// 目標の傾き
	const float maxBank = DirectX::XMConvertToRadians(20.0f);
	float targetX = inputForward * maxBank; // 前に倒す
	float targetZ = -inputRight * maxBank; // 右に倒す

	// いきなり傾かないようにLerpする
	float t = (std::min)(bankSpeed * elapsedTime, 1.0f);
	flightBankX += (targetX - flightBankX) * t;
	flightBankZ += (targetZ - flightBankZ) * t;

	angle.x = flightBankX;
	angle.z = flightBankZ;
}

// 腕のIK
void Player::ArmIK(float elapsedTime)
{
	if (status.isAvoid) return;
	// ＝＝＝＝＝＝ 今は迎撃の時だけ ＝＝＝＝＝＝
	
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 構えで止まっている間だけ腕を向ける
	bool isStance = !isGround
		&& currentOverlayState == "AS_Combo_Attack_04_01_Seq_0"
		&& animator->GetUpperCurrentSeconds() >= interceptStanceStopSeconds
		&& !(gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER);

	// 腕を敵に向ける時に補完する
	float targetWeight = isStance ? 1.0f : 0.0f;
	armAimWeight += (targetWeight - armAimWeight) * 10.0f * elapsedTime;
	if (armAimWeight <= 0.001f) { armAimWeight = 0.0f; return; }

	// 左肩と左手のボーンを取得する
	int armIndex = player->GetNodeIndex("upperarm_l");
	int handIndex = player->GetNodeIndex("hand_l");
	if (armIndex < 0 || handIndex < 0 || !enemy) return; // 無かったらreturn

	Model::Node& armNode = player->GetNodes().at(armIndex);
	Model::Node& handNode = player->GetNodes().at(handIndex);

	// 肩のローカル空間に変換する行列
	DirectX::XMMATRIX ArmWorld = DirectX::XMLoadFloat4x4(&armNode.worldTransform);
	DirectX::XMMATRIX InvArmWorld = DirectX::XMMatrixInverse(nullptr, ArmWorld);

	// 今の肩から手首への向き（ローカル）
	DirectX::XMVECTOR HandWorld = DirectX::XMVectorSet(
		handNode.worldTransform._41, handNode.worldTransform._42, handNode.worldTransform._43, 1.0f);
	DirectX::XMVECTOR LocalForward = DirectX::XMVector3Normalize(DirectX::XMVector3Transform(HandWorld, InvArmWorld));

	// 肩から敵への向き（ローカル）
	DirectX::XMFLOAT3 enemyPos = enemy->GetPosition();
	enemyPos.y += interceptAimHeight; // 敵の胸のあたりを狙う
	DirectX::XMVECTOR TargetLocal = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&enemyPos), InvArmWorld);
	DirectX::XMVECTOR LocalDirection = DirectX::XMVector3Normalize(TargetLocal);

	// 回転軸と角度
	DirectX::XMVECTOR Axis = DirectX::XMVector3Cross(LocalForward, LocalDirection);
	if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(Axis)) < 1e-6f) return; // すでに向いている
	Axis = DirectX::XMVector3Normalize(Axis);

	float dot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(LocalForward, LocalDirection));
	float rotAngle = acosf(std::clamp(dot, -1.0f, 1.0f));
	rotAngle = (std::min)(rotAngle, DirectX::XMConvertToRadians(interceptArmMaxAngle)); // 肩が壊れないように制限

	// 今の回転に足す
	DirectX::XMVECTOR CurrentRot = DirectX::XMLoadFloat4(&armNode.rotation);
	DirectX::XMVECTOR AddRot = DirectX::XMQuaternionRotationAxis(Axis, rotAngle * armAimWeight);
	DirectX::XMStoreFloat4(&armNode.rotation, DirectX::XMQuaternionMultiply(AddRot, CurrentRot));
}

// 狙う点を返す
DirectX::XMFLOAT3 Player::GetAimPoint(float range)
{
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// 始点と終点を求める
	DirectX::XMVECTOR Start = DirectX::XMLoadFloat3(&camera.GetEye());
	DirectX::XMVECTOR Front = DirectX::XMLoadFloat3(&camera.GetFront());
	DirectX::XMVECTOR End = DirectX::XMVectorAdd(Start, DirectX::XMVectorScale(Front, range));

	DirectX::XMFLOAT3 start, end, front;
	DirectX::XMStoreFloat3(&start, Start);
	DirectX::XMStoreFloat3(&end, End);
	DirectX::XMStoreFloat3(&front, Front);

	DirectX::XMFLOAT3 EPos = enemy->GetPosition();
	EPos.y += enemy->GetCollisionOffset();

	// 敵に当たったか調べる
	float enemyDist = range;
	DirectX::XMFLOAT3 enemyHitPos;
	bool hitEnemy = Collision::RayCastVsCapsule(start, end, EPos, enemy->GetCapsuleDirection(), enemy->GetHeight(), enemy->GetRadius(), enemyHitPos, enemyDist);

	// ステージに当たったか調べる
	const auto& traiangles = stage->GetTriangles();
	const auto& areas = stage->GetAreas();
	float stageDist = range;
	DirectX::XMFLOAT3 stageHitPos, stageHitNormal;
	bool hitStage = Collision::SphereCast(start, front, 0.01f, stageDist, traiangles, areas, stageHitPos, stageHitNormal, true);

	// 近い方の値を返す
	if (hitEnemy && hitStage)
	{
		return (enemyDist < stageDist) ? enemyHitPos : stageHitPos;
	}

	if (hitEnemy) return enemyHitPos;
	if (hitStage) return stageHitPos;

	// どちらでもなければ「end」を返す
	return end;
}

// 今ダウン中かを判定する
bool Player::IsDownState()
{
	// ダウン中ならtrueを返す
	if (status.isDown) return true;

	// サブグラフのDownの中にいる時は操作を出来ないようにする
	for (auto& idx : activeGraphStack)
	{
		if (transitionGraphs[idx].graphName == "Down") return true;
	}

	// Exit 直後、親グラフで "Down" ノードに留まっているフレームも操作不可
	return currentState == "Down";
}

// 蘇生したか判定する
bool Player::IsReviveState()
{
	// 死亡中ならtrueを返す
	if (status.isDead) return true;

	// サブグラフのReviveの中にいる時は操作を出来ないようにする
	for (auto& idx : activeGraphStack)
	{
		const std::string& name = transitionGraphs[idx].graphName;
		if (name == "Revive" || name == "Revive_Air") return true;
	}

	// Exit 直後、親グラフで "Revive系" ノードに留まっているフレームも操作不可
	return currentState == "Revive" || currentState == "Revive_Air";
}

// 操作できるか判定する
bool Player::CanControl()
{
	return !IsDownState() && !IsReviveState();
}

// 音を取得（無ければ自動ロード）する関数
AudioSource* Player::GetOrLoadSound(const std::string& soundName)
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
void Player::ShowDebugSwitch()
{
	// F4でアニメーション遷移エディターの表示をON/OFF
	static bool wasF4DownLastFrame = false;
	bool f4Down = (GetAsyncKeyState(VK_F4) & 0x8000) != 0;
	if (f4Down && !wasF4DownLastFrame) {
		showAnimarionTransitionEditor = !showAnimarionTransitionEditor;
	}
	wasF4DownLastFrame = f4Down;

	// F8でプレイヤーのパラメーターの表示をON/OFF
	static bool wasF8DownLastFrame = false;
	bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
	if (f8Down && !wasF8DownLastFrame) {
		showPlayerParameter = !showPlayerParameter;
	}
	wasF8DownLastFrame = f8Down;
}