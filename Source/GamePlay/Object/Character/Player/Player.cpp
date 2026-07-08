#include "Player.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"
#include "System/Audio/Audio.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h" 

#include "imgui_node_editor.h"
#include <imgui.h>


// =========================================================================
//  ライフサイクル・初期化
// =========================================================================

// 初期化
void Player::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデルの読み込み
	player = std::make_shared<Model>(device, "Data/Model/Player/Player/SK_SwordsmanGirl_02.gltf");
	weapon.model = std::make_shared<Model>(device, "Data/Model/Weapon/Player/Weapon/wizard_staff.gltf");
	weapon.scale.x = weapon.scale.y = weapon.scale.z = 0.01f;

	// アニメーターを生成
	animator = std::make_unique<Animator>();
	animator->Initialize(player);

	// パラメータ初期化
	moveSpeed = 7.0f;
	maxMoveSpeed = 7.0f;
	health = 1400.0f;
	MaxHealth = 1400.0f;
	Stamina = 150;
	MaxStamina = 150;
	currentPoise = 100.0f;
	maxPoise = 100.0f;
	baseAttackPower = 860.0f;
	invincibleTimer = 0.0f;

	weight = 0.5f;
	radius = 0.4f;
	height = 1.6f;
	debugOffset = 0.8;

	weapon.position = { 0.07f, 0.0f, 0.03f };
	weapon.angle = { -0.06, 0.23f, -1.13f };

	rootMotionNodeName = "root";
	upperBodyNodeName = "spine_01";

	// 各種システムの初期化（
	InitializeSequencer();          // シーケンサーの初期化
	InitializeAnimationNodeEditor(); // ノードエディターの初期化
	trail.Initialize();              // トレイルの初期化

	// アニメーションポーズと初期ステートの設定
	player->GetNodePoses(nodePoses);
	player->GetNodePoses(oldNodePoses);
	SetupInitialAnimationState();    // 初期アニメーションの設定
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

	// 今、文字入力中かどうかを取得
	bool isTyping = ImGui::GetIO().WantCaptureKeyboard;

	// ジャンプ入力を確定させる（文字入力中・プレビュー中は無効化）
	if (!isTyping && !isPreviewMode) {
		jumpPressed = (gamePad.GetButtonDown() & GamePad::BTN_A);
	}
	else {
		jumpPressed = false;
	}

	// タイマーを更新する関数
	UpdateTimers(elapsedTime);

	// プレイヤーの状態を計算する関数
	CalculateStatusParameters();

	if (gamePad.GetButtonDown() & GamePad::BTN_X || gamePad.GetButtonDown() & GamePad::BTN_Y)
	{
		MagicInput();
	}

	if (!isTyping && !isPreviewMode)
	{
		InputMove(elapsedTime);

		if (!isGuarding) Heal(elapsedTime);

		UpdateStateTransitions(elapsedTime);
		UpdateUpperBodyTransitions(elapsedTime);

	}
	else if (isTyping && !isPreviewMode)
	{
		// もし普通にゲームしている最中に文字入力を始めたら、足踏みしないようにIdleに戻す
		ChangeAnimationState("Idle");
	}

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

	// 根本と剣先を求める関数
	CalculationRootAndTip();

	// 魔法更新処理
	magicManager.Update(elapsedTime);

	// シーケンサーとアニメーションの再生を同期させる
	animSequence.Update(currentState, GetCurrentAnimationSeconds());
	SetBaseSpeed(animSequence.GetSpeed());

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	// サウンド再生
	UpdateSounds(this->currentState);

	// エリア移動制限
	AreaRestriction();

	// 移動時の背骨の回転角度を求める関数
	CalculateSpineRotation(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	player->UpdateTransform(transform);

	// 武器のアタッチメント処理
	UpdateWeaponAttachment("hand_r");
}

// 描画処理
void Player::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, player);
	renderer->Draw(ShaderId::PBR, weapon.model); // 杖

	// 魔法描画
	magicManager.Render(rc);
}

// GUI描画
void Player::DrawGUI()
{
	ImGui::Begin("Player");
	{
		ImGui::Text("currentOverlayState: %s", currentOverlayState.c_str());
		// パラメーター
		if (ImGui::CollapsingHeader("Parameter", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("MagicCoolDown:%f", magicCoolDownTimer);
			if (ImGui::Button(u8"HP前回"))
			{
				health = MaxHealth;
			}
			ImGui::Text("Health: %f.0", health);
			ImGui::Separator();
			ImGui::Text("Damage: %f.0", lastDamage);
			ImGui::Separator();
			ImGui::Text("currentPoise: %f.0", currentPoise);
			ImGui::Separator();
			ImGui::Text("InvincibleTimer: %f.0", invincibleTimer);
			ImGui::Separator();

			ImGui::Text("Move Speed: %f.0", moveSpeed); // 移動速度
			ImGui::Text("Velocity: %.2f, %.2f, %.2f", velocity.x, velocity.y, velocity.z);
			ImGui::Text("moveLength: %.3f", moveLength);
			ImGui::Text("EnemyfromDistance: %.3f", enemyFromDistance);
			if (IsGround())
			{
				ImGui::Text(u8"地面");
			}
			else
			{
				ImGui::Text(u8"空中");
			}
			ImGui::Separator();

			// 入力方向のデバッグ表示
			const char* debugDirNames[] = { "None", "Forward", "ForwardRight", "ForwardLeft", "Right", "Backward", "BackwardRight", "BackwardLeft", "Left" };
			ImGui::Text("Input Degree: %.2f", debug_degree);
			ImGui::Text("Input Dir: %s", debugDirNames[debug_dirIndex]);

			ImGui::Separator();

			ImGui::DragFloat("lerpSpeed", &lerpSpeed, 0.1f);
			ImGui::Text("smoothdMoveLength : %f.1", smoothMoveLength);

			if (ImGui::Button(u8"ポーション回復"))
			{
				potion = 14;
			}
			ImGui::Text("Potion: %d", potion);
			ImGui::Separator();
			if (isGuarding) {
				ImGui::Text(u8"ガード中");
			}
			else {
				ImGui::Text(u8"ガードしてない");
			}

			if (isAvoid) {
				ImGui::Text(u8"回避中");
			}
			else {
				ImGui::Text(u8"回避してない");
			}
		}

		// トランスフォーム情報
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Position", &position.x); // 位置

			DirectX::XMFLOAT3 a;
			a.x = DirectX::XMConvertToDegrees(angle.x);
			a.y = DirectX::XMConvertToDegrees(angle.y);
			a.z = DirectX::XMConvertToDegrees(angle.z);
			ImGui::DragFloat3("Angle", &a.x); // 回転
			// 表示用に度数法に変換した後、再度ラジアンで戻す処理
			angle.x = DirectX::XMConvertToRadians(a.x);
			angle.y = DirectX::XMConvertToRadians(a.y);
			angle.z = DirectX::XMConvertToRadians(a.z);

			ImGui::DragFloat3("Scale", &scale.x); // スケール
		}

		// 当たり判定情報
		if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Radius:", &radius, 0.1f); // 当たり判定の半径
			ImGui::DragFloat("Height:", &height, 0.1f); // 当たり判定の高さ
			ImGui::DragFloat("Collision Transform Offset:", &debugOffset, 0.1f);
		}

		// 武器のアタッチメント情報
		if (ImGui::CollapsingHeader("Weapon Attachment", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Position##1", &weapon.position.x, 0.01f);
			ImGui::DragFloat3("Angle##1", &weapon.angle.x, 0.01f);
			ImGui::DragFloat3("Scale##1", &weapon.scale.x, 0.01f);

			ImGui::DragFloat3("Weapon HitOffset##1", &weapon.weaponHitOffset.x, 0.1f);
			ImGui::DragFloat3("Weapon AngleOffset##1", &weapon.weaponAngleOffset.x, 0.1f);
			ImGui::DragFloat("Weapon Collision Radius##1", &weapon.weaponRadius, 0.1f);
			ImGui::DragFloat("Weapon Collision Height##1", &weapon.weaponHeight, 0.1f);

			ImGui::DragFloat(u8"武器の回転速度", &rotationSpeed, 0.1, 0.0f, 100.0f);
			ImGui::DragFloat(u8"剣の落下速度", &dropSpeed, 0.1, 0.0f, 100.0f);
			ImGui::DragFloat3(u8"回転中の杖の位置", &swingPosition.x, 0.1f);
			ImGui::DragFloat3(u8"回転中の杖の向き", &swingAngle.x, 0.1f, -360.0f, 360.0f);
			ImGui::DragFloat3(u8"回転軸", &swingOriginOffset.x, 0.1f);
		}

		// トレイル
		if (ImGui::CollapsingHeader("Weapon Trail", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Root Offset", &rootOffset.x, 0.01f);
			ImGui::DragFloat3("Tip Offset", &tipOffset.x, 0.01f);
		}

		// 魔法関係
		if (ImGui::CollapsingHeader("Magic", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::SliderFloat("Speed", &config.speed, 0.0f, 50.0f, "%.1f");
			ImGui::SliderFloat("Damage", &config.damage, 0.0f, 500.0f, "%.0f");
			ImGui::SliderFloat("Wait Time", &config.waitTimer, 0.0f, 2.0f, "%.2f");
			ImGui::SliderFloat("Life Time", &config.lifeTimer, 0.5f, 10.0f, "%.1f");

			ImGui::SliderInt("Level", &level, 1, 3);
		}

		// アニメーション遷移状態
		if (ImGui::CollapsingHeader("Animation/State", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto it = stateConfigs.find(currentState);

			// 見つかった場合のみ名前を表示する
			if (it != stateConfigs.end())
			{
				ImGui::Text("currentState: %s", it->second.animationName.c_str());
			}
			else
			{
				ImGui::Text("currentState: None (or Unknown)");
			}
		}

		// 移動時の背骨の傾き
		if (ImGui::CollapsingHeader(u8"背骨の傾き量", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat(u8"回転の強さ", &targetSpineAngle, 0.1f);
			ImGui::DragFloat(u8"回転速度", &spineAngleSpeed, 0.1f);
		}
	}
	ImGui::End();

	ImGui::Begin("Animation Transition Editor");
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
		ChangeAnimationState(editingState);
		isPreviewMode = true;
		ImGui::SetWindowFocus("Player Sequencer");
	}

	if (isPreviewMode)
	{
		ImGui::Begin("Preview Timeline");

		if (ImGui::Button(u8"▶ ゲームに戻る"))
		{
			isPreviewMode = false;
			ChangeAnimationState("Idle"); // プレビューを終えてIdleに戻す
		}
		ImGui::SameLine();

		// 現在の時間を取得
		float length = GetCurrentAnimationLength();

		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[%s]", editingState.c_str());
		ImGui::SameLine();
		ImGui::Text("  Time: %.2f / %.2f s", animationSeconds, length);

		// スライダーでアニメーションの時間を操作
		ImGui::SetNextItemWidth(-1);
		if (ImGui::SliderFloat("##animTime", &animationSeconds, 0.0f, length))
		{
			// スライダーを操作した瞬間は、補間を切らないとキャラがガタガタする
			isBlending = false;
		}

		ImGui::End();
	}

	// プレビュー中なら editingState を、そうじゃなければ今のステートをシーケンサーに渡す
	std::string displayState = isPreviewMode ? editingState : currentState;
	sequencerEditor.DrawEditor(
		animSequence,
		currentState,
		GetCurrentAnimationSeconds(),
		GetCurrentAnimationLength()
	);
}

// デバックプリミティブ描画
void Player::RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox)
{
	// プレイヤーの当たり判定
	if (!isAvoid)
	{
		DirectX::XMFLOAT4X4 capsuleTransform;
		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(position.x, position.y + debugOffset, position.z);
		DirectX::XMStoreFloat4x4(&capsuleTransform, S * T);

		renderer->DrawCapsule(capsuleTransform, radius, height, { 0, 1, 0, 1 });
	}

	// 武器の当たり判定
	if (showWeaponHitBox)
	{
		// 新しいGetActiveHitBoxesを使って描画する
		for (auto& info : GetActiveHitBoxes())
		{
			if (info.shape == HitBoxShape::Capsule)
			{
				// 位置を反映する簡易的な描画（Enemyと同じ方式）
				renderer->DrawCapsule(info.hitTransform, info.radius, info.height, { 1, 0, 0, 1 });
			}
			else if (info.shape == HitBoxShape::Sphere)
			{
				renderer->DrawSphere(info.position, info.radius, { 1, 0, 0, 1 });
			}
		}
	}

	// 魔法の当たり判定描画
	magicManager.RenderDebugPrimitive(renderer);
}


// =========================================================================
//  外部アクション命令
// =========================================================================

// 魔法射出
void Player::MagicInput()
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	if (gamePad.GetButtonDown() & GamePad::BTN_X)
	{
		if (magicCoolDownTimer > 0.0f) return;

		WindBladeMagic::Emit(&magicManager, position, angle.y, enemy, level, this->config);

		PlayActionGraph("State_Magic");

		if (level == 3)
		{
			magicCoolDownTimer = 1.0f;
		}
		else if (level == 2)
		{
			magicCoolDownTimer = 0.75f;
		}
		else
		{
			magicCoolDownTimer = 0.5f;
		}
	}
}

// 回復
void Player::Heal(float elapsedTime)
{
	GamePad gamePad = Input::Instance().GetGamePad();

	if ((gamePad.GetButtonDown() & GamePad::BTN_X) && healCoolDownTimer < 0.1f && potion > 0) {
		potion--; // 一つ使う
		healCoolDownTimer = 0.5f;
		regeneTimer = 1.0f;
	}


	if (healCoolDownTimer > 0.0f)
	{
		healCoolDownTimer -= elapsedTime;
	}

	if (regeneTimer > 0.0f)
	{
		float healStep = healValue * elapsedTime;

		health += healStep;

		regeneTimer -= elapsedTime;

		if (health > MaxHealth) health = MaxHealth;

		if (regeneTimer < 0.0f) regeneTimer = 0.0f;
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


// =========================================================================
//  セッター（インライン以外）
// =========================================================================

// ロックオン対象の位置を取得
void Player::SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos)
{
	lockOnTargetPos = pos;
}


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

// 今使ってるHitBoxを計算して返す関数
std::vector<Character::HitBoxInfo> Player::GetActiveHitBoxes() const
{
	std::vector<HitBoxInfo> result;
	auto state = GetCurrentState();

	for (auto& data : animSequence.GetAnimations()) {
		if (data.name != state) continue;

		for (auto& r : data.ranges)
		{
			if (r.isHitBox && animSequence.GetRange(r.name)) {
				HitBoxInfo info;
				info.shape = r.shapeType;
				info.radius = r.radius;
				info.height = r.height;
				info.damageRate = r.damageRate;
				info.poiseRate = r.poiseRate;
				info.invincibleTime = r.invincible;

				DirectX::XMMATRIX finalMatrix = DirectX::XMMatrixIdentity();
				bool isMatrixFound = false;

				// 名前で追従先を振り分ける！
				if (r.boneName == "Staff")
				{
					// 武器の計算済みワールド行列を読み込む
					DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

					// 【重要】スケール（縮小）成分を除去する！
					// これをやらないとシーケンサーのオフセット値まで一緒に縮んでしまう
					DirectX::XMVECTOR scale, rot, pos;
					DirectX::XMMatrixDecompose(&scale, &rot, &pos, weaponWorld);
					finalMatrix = DirectX::XMMatrixRotationQuaternion(rot) * DirectX::XMMatrixTranslationFromVector(pos);

					isMatrixFound = true;
				}
				else
				{
					// SwordやStaff以外（右手など）は従来通りキャラクターのボーンに追従
					int nodeIndex = player->GetNodeIndex(r.boneName.c_str());
					if (nodeIndex >= 0) {
						auto& node = player->GetNodes()[nodeIndex];
						DirectX::XMMATRIX local = DirectX::XMLoadFloat4x4(&node.globalTransform);
						DirectX::XMMATRIX playerWorld = DirectX::XMLoadFloat4x4(&GetTransform());
						finalMatrix = local * playerWorld;
						isMatrixFound = true;
					}
				}

				// 行列から位置と向きを抽出
				if (isMatrixFound) {
					// 1. エディタの回転（度数法）をラジアンに変換して、回転行列(R)を作る
					DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
						DirectX::XMConvertToRadians(r.angleOffset.x),
						DirectX::XMConvertToRadians(r.angleOffset.y),
						DirectX::XMConvertToRadians(r.angleOffset.z)
					);

					// 2. エディタの位置ズレから、平行移動行列(T)を作る
					DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(r.offset.x, r.offset.y, r.offset.z);

					// 3. 【最重要】回転 → 移動 → 親行列（剣）の順で掛け合わせて、最終的なヒットボックスの行列を作る！
					DirectX::XMMATRIX hitBoxMatrix = R * T * finalMatrix;

					DirectX::XMStoreFloat4x4(&info.hitTransform, hitBoxMatrix);

					// 行列の4行目（.r[3]）には、最終的なワールド座標が入っている
					DirectX::XMVECTOR pos = hitBoxMatrix.r[3];
					DirectX::XMStoreFloat3(&info.position, pos);

					// 行列の2行目（.r[1]）には、最終的なY軸方向（カプセルの上向き）のベクトルが入っている
					DirectX::XMVECTOR dir = DirectX::XMVector3Normalize(hitBoxMatrix.r[1]);
					DirectX::XMStoreFloat3(&info.direction, dir);
				}
				else {
					info.position = position;
					info.direction = { 0.0f, 1.0f, 0.0f };
				}

				result.push_back(info);
			}
		}
		break;
	}
	return result;
}


// =========================================================================
//  親子関係のオーバーライド
// =========================================================================

//着地したときに呼ばれる
void Player::OnLanding()
{
	jumpCount = 0;
}

// アニメーションのコールバック関数
void Player::OnStateChanged(const std::string& oldState, const std::string& newState)
{
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
	GamePad& gamePad = Input::Instance().GetGamePad();
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	if (!useRootMotion && !useRootMotionEx)
	{
		// 移動処理
		Move(moveVec.x, moveVec.z, moveSpeed);
	}

	// 回転処理
	if (lockOnTargetPos != nullptr)
	{
		float dx = lockOnTargetPos->x - position.x;
		float dz = lockOnTargetPos->z - position.z;
		float targetAngleY = atan2f(dx, dz); // 敵の方向に即座に向く

		float diff = targetAngleY - angle.y;
		diff = DirectX::XMScalarModAngle(diff);
		angle.y += diff * turnSpeed * elapsedTime;
	}
	else
	{
		Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
	}

	// 魔法レベル切り替え
	if (gamePad.GetButton() & GamePad::BTN_RIGHT_SHOULDER)
	{
		rbButtonHoldTime += elapsedTime;
		bool rbHold = rbButtonHoldTime >= RB_THRESHOLD; // 長押し判定

		if (rbHold)
		{
			// スティック入力値
			float rx = gamePad.GetAxisRX();
			float ry = gamePad.GetAxisRY();

			// 0.5以上なら入力受付(デットゾーン)
			float power = sqrt(rx * rx + ry * ry);
			if (power > 0.5)
			{
				// 入力角度を決める
				// アークタンジェントでラジアンを求める
				float radians = atan2f(rx, ry);

				// 度数法（0〜360度）に変換・補正する
				float StickAngle = DirectX::XMConvertToDegrees(radians);
				if (StickAngle < 0.0f) StickAngle += 360.0f;

				// 補正された角度（0〜360）に合わせて3等分する

				if (StickAngle >= 300.0f || StickAngle <= 60.0f)
				{
					level = 1; // 上
				}
				else if (StickAngle > 60.0f && StickAngle <= 180.0f)
				{
					level = 2; // 右下
				}
				else if (StickAngle > 180.0f && StickAngle < 300.0f)
				{
					level = 3; // 左下
				}
			}
		}
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
	for (int i = 0; i < (int)activeGraphStack.size(); i++)
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
		// ジャンプ処理
		if (nextState == "State_Jump" || nextState == "State_LockOn_Jump")
		{
			if (IsGround())
			{
				Jump(jumpSpeed);

				// 空中に飛び出すときに勢いを載せる
				if (moveLength > 0.1f)
				{
					float mvX = moveVec.x / moveLength;
					float mvZ = moveVec.z / moveLength;

					// 走ってるときの速度を掛けてVelocityに送る
					float jumpForwardSpeed = maxMoveSpeed;
					velocity.x = mvX * jumpForwardSpeed;
					velocity.z = mvZ * jumpForwardSpeed;
				}
				else
				{
					velocity.x = 0.0f;
					velocity.z = 0.0f;
				}

				jumpCount++;
				bufferedButton &= ~GamePad::BTN_A; // 先行入力の記憶をリセット
			}
		}

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
				case TransitionActionType::SetIsAvoid: isAvoid = (action.value != 0.0f); break;
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
				activeGraphStack.push_back(targetIndex);

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
							case TransitionActionType::SetIsAvoid: isAvoid = (action.value != 0.0f); break;
							}
						}
					}
					// 最初のステートへ切り替え！
					if (firstState != "")
					{
						ChangeAnimationState(firstState);
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
				if (firstState != "") ChangeAnimationState(firstState);
			}
		}
		else if (nextNode && nextNode->type == NodeType::Mix)
		{
			// 逆探知関数でAとBのアニメーション名を取ってくる
			std::string stateA = currentGraph->GetInputNodeName(nextState, InputSlot::A);
			std::string stateB = currentGraph->GetInputNodeName(nextState, InputSlot::B);

			// 名前からアニメーション番号に変換する
			int animIndexA = GetModel()->GetAnimationIndex(stateConfigs[stateA].animationName.c_str());
			int animIndexB= GetModel()->GetAnimationIndex(stateConfigs[stateB].animationName.c_str());

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
	for (int i = 0; i < (int)upperGraphStack.size(); i++)
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
		// ジャンプ処理
		if (nextState == "State_Jump" || nextState == "State_LockOn_Jump")
		{
			Jump(jumpSpeed);

			// 空中に飛び出すときに勢いを載せる
			if (moveLength > 0.1f)
			{
				float mvX = moveVec.x / moveLength;
				float mvZ = moveVec.z / moveLength;

				// 走ってるときの速度を掛けてVelocityに送る
				float jumpForwardSpeed = maxMoveSpeed;
				velocity.x = mvX * jumpForwardSpeed;
				velocity.z = mvZ * jumpForwardSpeed;
			}
			else
			{
				velocity.x = 0.0f;
				velocity.z = 0.0f;
			}

			jumpCount++;
			bufferedButton &= ~GamePad::BTN_A; // 先行入力の記憶をリセット
		}

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
				case TransitionActionType::SetIsAvoid: isAvoid = (action.value != 0.0f); break;
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
							case TransitionActionType::SetIsAvoid: isAvoid = (action.value != 0.0f); break;
							}
						}
					}
					// 最初のステートへ切り替え！
					if (firstState != "")
					{
						ChangeAnimationState(firstState);
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
				if (firstState != "") ChangeAnimationState(firstState);
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

	isGuarding = (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER);

	UpdateStateBehavior();

	TransitionContext ctx;
	ctx.position = GetPosition();
	ctx.animSeconds = GetCurrentAnimationSeconds();
	ctx.animLength = GetCurrentAnimationLength();
	ctx.moveLength = smoothMoveLength;
	ctx.enemyFromdistance = enemyFromDistance;
	ctx.buttonDown = gamePad.GetButtonDown() | bufferedButton;
	ctx.buttonHeld = gamePad.GetButton();
	ctx.buttonUp = gamePad.GetButtonUp();
	ctx.IsGround = IsGround();
	ctx.isLockOn = (lockOnTargetPos != nullptr);
	ctx.havePotion = (potion > 0);
	ctx.healCooldownReady = (healCoolDownTimer < 0.1f);
	ctx.isGuarding = isGuarding;
	ctx.IsCancelable = animSequence.GetRange("Cancelable");
	ctx.IsFalling = falling;
	ctx.inputDir = CalculateRelativeInputDir(); 

	return ctx;
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

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);
}

// プレイヤーの状態を計算する関数
void Player::CalculateStatusParameters()
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	falling = (velocity.y < -0.1f);

	// 滞空処理
	if (falling && gamePad.GetButton() & GamePad::BTN_A)
	{
		SetGravity(-1.0f);
	}
	else if (falling)
	{
		SetGravity(-25.0f);
	}
	else
	{
		SetGravity(-9.8f);
	}

	// 敵との距離の計算
	DirectX::XMVECTOR Ppos = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR Epos = DirectX::XMLoadFloat3(&enemy->GetPosition());
	DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Epos, Ppos);
	enemyFromDistance = DirectX::XMVectorGetX(DirectX::XMVector3Length(Vec));
}

// 剣先と根本を求める関数
void Player::CalculationRootAndTip()
{
	for (int i = 0; i < 2; ++i)
	{
		// 剣の根本・先端の座標を計算
		DirectX::XMVECTOR RootOffset = DirectX::XMLoadFloat3(&rootOffset);
		DirectX::XMVECTOR TipOffset = DirectX::XMLoadFloat3(&tipOffset);

		DirectX::XMMATRIX W = DirectX::XMLoadFloat4x4(&weapon.transform);

		DirectX::XMVECTOR Root = DirectX::XMVector3TransformCoord(RootOffset, W);
		DirectX::XMVECTOR Tip = DirectX::XMVector3TransformCoord(TipOffset, W);

		DirectX::XMFLOAT3 rootF3, tipF3;
		DirectX::XMStoreFloat3(&rootF3, Root);
		DirectX::XMStoreFloat3(&tipF3, Tip);

		trail.Update(rootF3, tipF3);
	}
}

// 移動時の背骨の回転角度を求める関数
void Player::CalculateSpineRotation(float elapsedTime)
{
	// 今回の背骨の回転角度と前回の背骨の回転角度の差分を求める
	float deltaAngleY = angle.y - previousAngleY;

	// XMScalarModAngle : 角度を -π ～ +π の範囲に正規化する関数
	deltaAngleY = DirectX::XMScalarModAngle(deltaAngleY);

	// 目標角度を求める
	float targetBankAngle = deltaAngleY * targetSpineAngle;

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