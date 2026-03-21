#include "Player.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"

#include <imgui.h>

// 初期化
void Player::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// プレイヤーモデル読み込み
	player = std::make_shared<Model>(device, "Data/Model/Player/SKM_DKM_Full.gltf");

	// 武器モデル読み込み
	weapon.model = std::make_shared<Model>(device, "Data/Model/Weapon/Player/GreatSword.gltf");
	weapon.scale.x = weapon.scale.y = weapon.scale.z = 0.1f;

	 // プレイヤーパラメーター初期化
	moveSpeed = 2.0f;
	maxMoveSpeed = 7.0f;
	MaxHealth = 1400.0f;
	health = 1400.0f;
	maxPoise = 100.0f;
	currentPoise = 100.0f;

	baseAttackPower = 860.0f;
	invincibleTimer = 0.0f;


	// 当たり判定パラメーター初期化
	weight = 0.5f;
	radius = 0.7f;
	height = 1.6f;
	debugOffset = 0.8;
	weapon.weaponHitOffset = { -0.35, -1.05, -0.1 };
	weapon.weaponAngleOffset = { 0.06, 6.6, 0.23 };
	weapon.weaponRadius = 0.3f;
	weapon.weaponHeight = 1.7f;

	// 武器のパラメーター初期化
	weapon.position = { 0.34, 1.02, 0.04 };
	weapon.angle = { 0, 0, 2.89 };

	// Jsonファイルの初期化
	InitializeAttackData();

	// アニメーション設定
	AnimationStateManager<PlayerAnimationState>::Instance();
	player->GetNodePoses(nodePoses);
	player->GetNodePoses(oldNodePoses);
	ChangeAnimationState(PlayerAnimationState::Idle);
}

// 攻撃とかの情報を初期化(Jsonファイルの初期化)
void Player::InitializeAttackData()
{
	// シーケンサーの初期化
	animSequence.SetModel(player);

	// Jsonがあれば読み込む、無ければデフォルト値を設定
	std::ifstream file("Data/Json/Player/AttackData/AttackSequence.json");
	if (file.is_open())
	{
		file.close();
		animSequence.Load("Data/Json/Player/AttackData/AttackSequence.json");
	}
	else
	{
		animSequence.attackData[PlayerAnimationState::Attack_01] = {
		{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[PlayerAnimationState::Attack_02] = {
		  { 55, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[PlayerAnimationState::Charge_Attack] = {
		  { 55, 82, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		  { 100, 127, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[PlayerAnimationState::Run_Attack] = {
		  { 55, 100, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[PlayerAnimationState::Guard_Counter] = {
		  { 55, 82, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
		animSequence.attackData[PlayerAnimationState::Jump_Attack] = {
		  { 15, 42, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		};
	}
}

// 更新処理
void Player::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 一番最初にジャンプ入力を確定させる
	jumpPressed = (gamePad.GetButtonDown() & GamePad::BTN_A) && CanJump();

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// 入力処理
	InputMove(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// 武器のアタッチメント処理
	WeaponAttachment();

	// 状態遷移更新処理
	UpdateStateTransitions(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	// 回復
	Heal(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	player->UpdateTransform(transform);
}

// 描画処理
void Player::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, player); 
	renderer->Draw(ShaderId::PBR, weapon.model);
}

// GUI描画
void Player::DrawGUI()
{
	ImGui::Begin("Player");
	{
		// パラメーター
		if (ImGui::CollapsingHeader("Parameter", ImGuiTreeNodeFlags_DefaultOpen))
		{
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
			ImGui::Text("Potion: %d", Potion);
			ImGui::Separator();



			ImGui::DragFloat("Move Speed:", &moveSpeed, 1.0f, 0, 10); // 移動速度
			ImGui::Text("Velocity: %.2f, %.2f, %.2f", velocity.x, velocity.y, velocity.z);
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

			ImGui::DragFloat3("Weapon HitOffset", &weapon.weaponHitOffset.x, 0.1f);
			ImGui::DragFloat3("Weapon AngleOffset", &weapon.weaponAngleOffset.x, 0.1f);
			ImGui::DragFloat("Weapon Collision Radius", &weapon.weaponRadius, 0.1f);
			ImGui::DragFloat("Weapon Collision Height", &weapon.weaponHeight, 0.1f);
		}
		// 武器のアタッチメント情報
		if (ImGui::CollapsingHeader("Animation/State", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("currentState: %d", (int)currentState);
			ImGui::Text("Stick Angle: %.2f deg", debug_degree);
			ImGui::Text("Table Index: %d", debug_dirIndex);
			const char* directionNames[] = { "F", "R", "BR", "BL", "L" };
			if (debug_degree > 0.1f || debug_degree < -0.1f || debug_dirIndex != 0) {
				ImGui::Text("Determined Dir: %s", directionNames[debug_dirIndex]);
			}
			else {
				ImGui::Text("Determined Dir: Idle/None");
			}
		}


	}
	ImGui::End();

	ImGui::Begin("Player Attack Sequencer");
	{
		auto& AnimSequence = GetAnimSequence();
		auto& manager = AnimationStateManager<PlayerAnimationState>::Instance();

		float totalSec = AnimSequence.GetAnimationLength(AnimSequence.currentState);
		ImGui::Text(u8"総秒数: %.2f秒  (バーの数値 ÷ 100 = 秒)", totalSec);
		if (selectedEntry >= 0)
		{
			auto& tracks = AnimSequence.CurrentTracks();
			if (selectedEntry < (int)tracks.size())
			{
				auto& t = tracks[selectedEntry];
				ImGui::Text(u8"選択中: %.2f秒 〜 %.2f秒",
					t.GetStartSeconds(), t.GetEndSeconds());
			}
		}

		// 選択中のステートにトラックを追加するボタン
		if (ImGui::Button(u8"+ 追加"))
		{
			AnimSequence.attackData[AnimSequence.currentState].push_back(
				{ 0, 50, u8"新しい判定", 0xFF0000FF, TrackType::HitBox }
			);
		}

		// 保存・読み込みボタン
		if (ImGui::Button(u8"保存"))
			AnimSequence.Save("Data/Json/Player/AttackData/AttackSequence.json");
		ImGui::SameLine();
		if (ImGui::Button(u8"読み込み"))
			AnimSequence.Load("Data/Json/Player/AttackData/AttackSequence.json");

		for (auto& [state, tracks] : AnimSequence.attackData)
		{
			const AnimationConfig* config = manager.GetConfig(state);
			if (ImGui::Button(config->animationName.c_str()))
			{
				AnimSequence.currentState = state;
			}
			ImGui::SameLine();
		}
		ImGui::NewLine();

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
void Player::RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox)
{
	// プレイヤーの当たり判定
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
		DirectX::XMFLOAT4X4 weaponTransform;

		DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

		// 武器の現在の位置と回転だけを取り出す（スケールを無視する）
		DirectX::XMVECTOR scale, rot, pos;
		DirectX::XMMatrixDecompose(&scale, &rot, &pos, weaponWorld);
		DirectX::XMMATRIX baseMatrix = DirectX::XMMatrixRotationQuaternion(rot) * DirectX::XMMatrixTranslationFromVector(pos);

		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
			weapon.angle.x + weapon.weaponAngleOffset.x,
			weapon.angle.y + weapon.weaponAngleOffset.y,
			weapon.angle.z + weapon.weaponAngleOffset.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			weapon.position.x + weapon.weaponHitOffset.x,
			weapon.position.y + weapon.weaponHitOffset.y,
			weapon.position.z + weapon.weaponHitOffset.z);
		DirectX::XMMATRIX WorldWeapon = R * T * baseMatrix;
		DirectX::XMStoreFloat4x4(&weaponTransform, WorldWeapon);

		renderer->DrawCapsule(weaponTransform, weapon.weaponRadius, weapon.weaponHeight, { 1, 0, 0, 1 });
	}
}

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

// 回復
void Player::Heal(float elapsedTime)
{
	GamePad gamePad = Input::Instance().GetGamePad();

	if ((gamePad.GetButtonDown() & GamePad::BTN_X) && HealCoolDownTimer < 0.1f && Potion > 0) {
		Potion--; // 一つ使う
		HealCoolDownTimer = 0.5f;
		RegeneTimer = 1.0f;
	}


	if (HealCoolDownTimer > 0.0f)
	{
		HealCoolDownTimer -= elapsedTime;
	}

	if(RegeneTimer > 0.0f)
	{
		float healStep = HealValue * elapsedTime;

		health += healStep;

		RegeneTimer -= elapsedTime;

		if (health > MaxHealth) health = MaxHealth;

		if (RegeneTimer < 0.0f) RegeneTimer = 0.0f;
	}
}

// ロックオン対象の位置を取得
void Player::SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos) 
{
	lockOnTargetPos = pos;
}

//着地したときに呼ばれる
void Player::OnLanding()
{
	jumpCount = 0;
}

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
		cameraFrontX /= cameraRightLength;
		cameraFrontZ /= cameraRightLength;
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

	// 移動処理
	Move(moveVec.x, moveVec.z, moveSpeed);

	// 回転処理

	if (currentState == PlayerAnimationState::Run)
	{
		Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
	}
	else if (lockOnTargetPos != nullptr)
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

	if (jumpPressed)
	{
		if (jumpCount < jumpLimit)
		{
			jumpCount++;
			Jump(jumpSpeed);
		}
	}
}

// ジャンプ出来るかどうか
bool Player::CanJump() const
{
	// Jump_End 中はジャンプ不可（着地の瞬間のフレームのズレ対策）
	if (currentState == PlayerAnimationState::Jump_End)
	{
		return IsAnimationOutTimeRange(0.238f);
	}

	if (currentState == PlayerAnimationState::Charge_Attack_Start) return false;
	// 溜め攻撃の開始直後などは絶対にジャンプ不可
	if (currentState == PlayerAnimationState::Charge_Attack_Start) return false;

	// 通常攻撃の場合
	if (currentState == PlayerAnimationState::Attack_01) 
	{
		return IsAnimationOutTimeRange(0.82f);
	}
	else if (currentState == PlayerAnimationState::Attack_02)
	{
		return IsAnimationOutTimeRange(0.82f);
	}
	else if (currentState == PlayerAnimationState::Charge_Attack)
	{
		return IsAnimationOutTimeRange(1.2f);

	}
	else if (currentState == PlayerAnimationState::Run_Attack)
	{
		return IsAnimationOutTimeRange(1.248f);

	}
	else if (currentState == PlayerAnimationState::Jump_Attack)
	{
		return IsAnimationOutTimeRange(0.986);

	}

	return true;
}

// 状態遷移更新処理
void Player::UpdateStateTransitions(float elapsedTime)
{
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	float moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
	GamePad& gamePad = Input::Instance().GetGamePad();

	PlayerAnimationState moveState = DetermineWalkState(); // 歩きの遷移条件を取得
	PlayerAnimationState rollState = DetermineRollState(); // 回避の遷移条件を取得

	// 走ってる途中にローリングするのを防ぐため人次フレームで0にする

	bool bHold = bButtonHoldTime >= RUN_THRESHOLD; // 長押し判定

	// 短押し判定：離した瞬間 かつ 長押しじゃなかった
	// ※ GetButtonUp() に戻すことで「押した瞬間にRunとRollが競合する」問題を解消
	bool bTap = (gamePad.GetButtonUp() & GamePad::BTN_B)
		&& !bHold;

	bool rtHold = rtButtonHoldTime >= ATTACK_THRESHOLD; // 長押し判定
	bool rtTap = (gamePad.GetButtonUp() & GamePad::BTN_RIGHT_TRIGGER)
		&& !rtHold; // 短押し判定

	// Bボタンの長押し時間を計測
	if (gamePad.GetButton() & GamePad::BTN_B)
		bButtonHoldTime += elapsedTime;
	else
		bButtonHoldTime = 0.0f;

	// RTトリガーの長押し時間を計測
	if (gamePad.GetButton() & GamePad::BTN_RIGHT_TRIGGER)
		rtButtonHoldTime += elapsedTime;
	else
		rtButtonHoldTime = 0.0f;

	switch (currentState)
	{
		// ===== 待機 =====
	case PlayerAnimationState::Idle:
	{
		if (jumpPressed)
		{
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
		}
		else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Jog_F); // ガード(小走り)に遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Walk_F); // ガード(歩き)に遷移
			}
			else
			{
				ChangeAnimationState(PlayerAnimationState::Guard); // ガードに遷移
			}
		}
		else if (moveLength > 0.5f)
		{
			mode = MoveMode::Jog;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の小走りへ遷移
			else
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
		}
		else if (moveLength > 0.1f && bHold)
		{
			ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
		}
		else if (moveLength > 0.1f && moveLength < 0.5f)
		{
			mode = MoveMode::Walk;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の歩きに遷移
			else
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
		}
		else if (moveLength > 0.1f && bTap)
		{
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(rollState); // ロックオン中の回避に遷移
			else
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
		}
		else if (bTap)
		{
			ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
		}
		else if (rtTap)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
		}
		else if (rtHold)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_X && HealCoolDownTimer < 0.1f && Potion > 0)
		{
			StartOverlayAnimation(PlayerAnimationState::Heal); // 回復に遷移
		}

		break;
	}

	// ===== 歩き =====
	case PlayerAnimationState::Walk_B:
	case PlayerAnimationState::Walk_BL:
	case PlayerAnimationState::Walk_BR:
	case PlayerAnimationState::Walk_F:
	case PlayerAnimationState::Walk_L:
	case PlayerAnimationState::Walk_R:
	{
		moveSpeed = 3.0f;

		if (moveLength > 0.1f) {
			if (lockOnTargetPos != nullptr) {
				if (currentState != moveState) {
					ChangeAnimationState(moveState);
					break;
				}
			}
			else {
				if (currentState != PlayerAnimationState::Walk_F) {
					ChangeAnimationState(PlayerAnimationState::Walk_F);
					break;
				}
			}
		}

		if (moveLength <= 0.1f) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移
		else if (jumpPressed)
		{
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
		}
		else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Jog_F); // ガード(小走り)に遷移
				break;
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Walk_F); // ガード(歩き)に遷移
				break;
			}
			else
			{
				ChangeAnimationState(PlayerAnimationState::Guard); // ガードに遷移
				break;
			}
		}
		else if (moveLength > 0.1f && bHold)
		{
			ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
		}
		else if (moveLength > 0.5f)
		{
			mode = MoveMode::Jog;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の小走りへ遷移
			else
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
		}
		else if (moveLength > 0.1f && bTap)
		{
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(rollState); // ロックオン中の回避に遷移
			else
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
		}
		else if (bTap)
		{
			ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
		}
		else if (rtTap)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
		}
		else if (rtHold)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_X && HealCoolDownTimer < 0.1f && Potion > 0) {
			mode = MoveMode::Walk;
			isOverlayPlaying = true; // 上半身路下半身の別々のアニメーションを起動する
			StartOverlayAnimation(PlayerAnimationState::Heal); // 上半身のアニメーションを回復にする
			ChangeAnimationState(moveState, true); // 下半身のアニメーションを歩きにする
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_X && HealCoolDownTimer < 0.1f && Potion > 0) {
			mode = MoveMode::Jog;
			isOverlayPlaying = true; // 上半身路下半身の別々のアニメーションを起動する
			StartOverlayAnimation(PlayerAnimationState::Heal); // 上半身のアニメーションを回復にする
			ChangeAnimationState(moveState, true); // 下半身のアニメーションを歩きにする
		}

		break;
	}

	// ===== 小走り =====
	case PlayerAnimationState::Jog_B:
	case PlayerAnimationState::Jog_BL:
	case PlayerAnimationState::Jog_BR:
	case PlayerAnimationState::Jog_F:
	case PlayerAnimationState::Jog_L:
	case PlayerAnimationState::Jog_R:
	{
		moveSpeed = 5.0f;

		if (moveLength > 0.1f) {
			if (lockOnTargetPos != nullptr) {
				// ロックオン中なら、入力方向(moveState)と一致しているかチェック
				if (currentState != moveState) {
					ChangeAnimationState(moveState);
					break;
				}
			}
			else {
				// ロックオンなしなら、Jog_Fと一致しているかチェック
				if (currentState != PlayerAnimationState::Jog_F) {
					ChangeAnimationState(PlayerAnimationState::Jog_F);
					break;
				}
			}
		}



		if (moveLength <= 0.1f) ChangeAnimationState(PlayerAnimationState::Idle);
		else if (gamePad.GetButtonDown() & GamePad::BTN_X && HealCoolDownTimer < 0.1f && Potion > 0) {
			mode = MoveMode::Walk;
			isOverlayPlaying = true; // 上半身路下半身の別々のアニメーションを起動する
			StartOverlayAnimation(PlayerAnimationState::Heal); // 上半身のアニメーションを回復にする
			ChangeAnimationState(moveState); // 下半身のアニメーションを小走りにする
		}
		else if (jumpPressed)
		{
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
		}
		else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Jog_F); // ガード(小走り)に遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Walk_F); // ガード(歩き)に遷移
			}
		}
		else if (moveLength > 0.1f && bHold)
		{
			ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
		}
		else if (moveLength > 0.1f && moveLength < 0.5f)
		{
			mode = MoveMode::Walk;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の歩きに遷移
			else
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
		}
		else if (moveLength > 0.1f && bTap)
		{
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(rollState); // ロックオン中の回避に遷移
			else
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
		}
		else if (bTap)
		{
			ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
		}
		else if (rtTap)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
		}
		else if (rtHold)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
		}

		break;
	}

	// ===== 走り =====
	case PlayerAnimationState::Run:
	{
		moveSpeed = 7.0f;

		if (moveLength < 0.1f)
		{
			ChangeAnimationState(PlayerAnimationState::Run_Stop); // 急停止に遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Run_Attack); // ダッシュ攻撃に遷移
			break;
		}
		else if (jumpPressed)
		{
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプ攻撃に遷移
		}
		else if (bHold && gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			baseSpeed = 1.2f;
			ChangeAnimationState(PlayerAnimationState::Guard_Jog_F);
		}
		// ジャンプも攻撃もしていない時だけ、BボタンによるRun維持を行う
		else if (bHold)
		{
			break;
		}
		else if (moveLength > 0.5f)
		{
			ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
		}
		else if (moveLength > 0.1f && moveLength < 0.5f)
		{
			ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
		}

		break;
	}

	case PlayerAnimationState::Run_Stop:
	{
		moveSpeed = 0.0f;

		if (IsAnimationOutTimeRange(0.5f))
		{
			if (moveLength <= 0.1f) ChangeAnimationState(PlayerAnimationState::Idle);
			if (jumpPressed)
			{
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
			else if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
			else if (moveLength > 0.1f && bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
			}
			else if (bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			}
			else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			}
			else if (rtTap)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			}
			else if (rtHold)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			}
		}

		break;
	}

	// ===== 回避 =====
	case PlayerAnimationState::Roll_BL:
	case PlayerAnimationState::Roll_BR:
	case PlayerAnimationState::Roll_F:
	case PlayerAnimationState::Roll_L:
	case PlayerAnimationState::Roll_R:
	{
		turnSpeed = DirectX::XMConvertToRadians(0);
		moveSpeed = 0;

		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle);

		if (IsAnimationOutTimeRange(0.568f))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (jumpPressed)
			{
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
			else if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
			else if (bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			}
			else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			}
			else if (rtTap)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			}
			else if (rtHold)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			}
		}

		break;
	}

	// ===== バックステップ =====
	case PlayerAnimationState::Dodge:
	{
		moveSpeed = 0.0f;
		turnSpeed = DirectX::XMConvertToRadians(0);

		if (IsAnimationOutTimeRange(0.549))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (moveLength < 0.1f) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移
			if (jumpPressed)
			{
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
			else if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
			else if (moveLength > 0.1f && bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
			}
			else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			}
			else if (rtTap)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			}
			else if (rtHold)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			}
		}

		break;
	}

	// ===== 攻撃 =====
	case PlayerAnimationState::Attack_01:
	{
		moveSpeed = 0.0f;

		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移

		if (IsAnimationOutTimeRange(0.82f))
		{
			turnSpeed = DirectX::XMConvertToRadians(0);
			if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER) ChangeAnimationState(PlayerAnimationState::Attack_02); // 攻撃2に遷移
			else if (rtTap) ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 溜め攻撃へ遷移
			else if (rtHold) ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			else if (moveLength > 0.1f && bTap) ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避へ遷移
			else if (bTap) ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			else if (jumpPressed)
			{
				moveSpeed = 2.0f;
				turnSpeed = DirectX::XMConvertToRadians(720);
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
		}

		if (IsAnimationOutTimeRange(1.5f))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
		}

		break;
	}

	case PlayerAnimationState::Attack_02:
	{
		moveSpeed = 0.0f;

		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移

		if (IsAnimationOutTimeRange(0.82f))
		{
			turnSpeed = DirectX::XMConvertToRadians(0);

			if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER) ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			else if (rtTap) ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 溜め攻撃へ遷移
			else if (rtHold) ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			else if (moveLength > 0.1f && bTap) ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避へ遷移
			else if (bTap) ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			else if (jumpPressed)
			{
				moveSpeed = 2.0f;
				turnSpeed = DirectX::XMConvertToRadians(720);
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
		}

		if (IsAnimationOutTimeRange(1.5f))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
		}

		break;
	}

	// ===== 溜め攻撃 =====
	case PlayerAnimationState::Charge_Attack:
	{
		moveSpeed = 0.0f;

		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移

		if (IsAnimationOutTimeRange(1.2f))
		{
			turnSpeed = DirectX::XMConvertToRadians(0);

			if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER) ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			else if (moveLength > 0.1f && bTap) ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避へ遷移
			else if (bTap) ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			else if (jumpPressed)
			{
				moveSpeed = 2.0f;
				turnSpeed = DirectX::XMConvertToRadians(720);
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
		}

		if (IsAnimationOutTimeRange(1.5f))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
		}

		break;
	}

	case PlayerAnimationState::Charge_Attack_Start:
	{
		moveSpeed = 0.0f;
		turnSpeed = DirectX::XMConvertToRadians(0);

		AnimationLerp(0.188f, 0.583f, 0.4f);

		if (IsAnimationFinished())
		{
			SetBaseSpeed(1.0f);
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 溜め攻撃へ遷移
		}
		else if (!rtHold)
		{
			SetBaseSpeed(1.0f);
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 溜め攻撃へ遷移
		}

		break;
	}

	// ===== ダッシュ攻撃 =====
	case PlayerAnimationState::Run_Attack:
	{
		moveSpeed = 0.0f;

		AnimationLerp(0.0f, 0.319f, 0.4f);
		SetBaseSpeed(1.0f);

		if (IsAnimationOutTimeRange(1.248f))
		{
			turnSpeed = DirectX::XMConvertToRadians(0);

			if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER) ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			else if (rtTap) ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 溜め攻撃へ遷移
			else if (rtHold) ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			else if (moveLength > 0.1f && bTap) ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避へ遷移
			else if (bTap) ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			else if (jumpPressed)
			{
				moveSpeed = 2.0f;
				turnSpeed = DirectX::XMConvertToRadians(720);
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
		}

		if (IsAnimationOutTimeRange(2.466f))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
		}
		break;
	}

	// ===== ガードカウンター攻撃 =====
	case PlayerAnimationState::Guard_Counter:
		break;

		// ===== ジャンプ攻撃 =====
	case PlayerAnimationState::Jump_Attack_Start:
	{
		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Jump_Attack_Loop);
		break;
	}
	case PlayerAnimationState::Jump_Attack_Loop:
	{
		if (position.y <= 0.01f) ChangeAnimationState(PlayerAnimationState::Jump_Attack);
		break;
	}
	case PlayerAnimationState::Jump_Attack:
	{
		moveSpeed = 0.0f;
		turnSpeed = DirectX::XMConvertToRadians(0);

		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移

		if (IsAnimationOutTimeRange(0.986))
		{
			if (moveLength > 0.1f && bTap) ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
			else if (bTap) ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER) ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			else if (rtTap) ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			else if (rtHold) ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			else if (jumpPressed)
			{
				moveSpeed = 2.0f;
				turnSpeed = DirectX::XMConvertToRadians(720);
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
		}

		if (IsAnimationOutTimeRange(1.624f))
		{
			turnSpeed = DirectX::XMConvertToRadians(720);

			if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移

			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
		}

		break;
	}

	// ===== ジャンプ =====
	case PlayerAnimationState::Jump_Start:
	{
		if (position.y > 0.1f) ChangeAnimationState(PlayerAnimationState::Jump_Loop); // 空中待機へ遷移
		break;
	}
	case PlayerAnimationState::Jump_Loop:
	{
		if (position.y > 0.1f && gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER) ChangeAnimationState(PlayerAnimationState::Jump_Attack_Start);
		else if (position.y <= 0.01f)
		{
			animationBlendSecondsLength = 0.05f;
			ChangeAnimationState(PlayerAnimationState::Jump_End); // 着地へ遷移
		}
		break;
	}
	case PlayerAnimationState::Jump_End:
	{
		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移

		if (IsAnimationOutTimeRange(0.089))
		{
			if (jumpPressed)
			{
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
			else if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
			else if (moveLength > 0.1f && bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
			}
			else if (bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			}
			else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			}
			else if (rtTap)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			}
			else if (rtHold)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			}
		}

		break;
	}

	// ===== ガード(立ち) =====
	case PlayerAnimationState::Guard:
	{
		moveSpeed = 0.0f;

		if (jumpPressed)
		{
			moveSpeed = 2.0f;
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			break;
		}
		else if (moveLength > 0.1f && bTap)
		{
			ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
			break;
		}
		else if (bTap)
		{
			ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			break;
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			break;
		}
		else if (rtTap)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			break;
		}
		else if (rtHold)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			break;
		}
		else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			if (moveLength > 0.5f)
			{
				moveSpeed = 5.0f;
				ChangeAnimationState(PlayerAnimationState::Guard_Jog_F); // ガード(小走り)に遷移
				break;
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				moveSpeed = 3.0f;
				ChangeAnimationState(PlayerAnimationState::Guard_Walk_F); // ガード(歩き)に遷移
				break;
			}
		}

		if (!(gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER))
		{
			if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
			else
			{
				ChangeAnimationState(PlayerAnimationState::Idle); // 待機へ遷移
			}
		}

		break;
	}

	// ===== ガード(小走り) =====
	case PlayerAnimationState::Guard_Jog_B:
	case PlayerAnimationState::Guard_Jog_BL:
	case PlayerAnimationState::Guard_Jog_BR:
	case PlayerAnimationState::Guard_Jog_F:
	case PlayerAnimationState::Guard_Jog_L:
	case PlayerAnimationState::Guard_Jog_R:
	{
		if (bHold) moveSpeed = 7.0f; // ガード中の走り
		else moveSpeed = 5.0f;

		if (jumpPressed)
		{
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
		}
		// ガード中走り(bHold)のときはbTapを無効化して回避に遷移させない
		else if (moveLength > 0.1f && bTap && !bHold)
		{
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(rollState); // ロックオン中の回避に遷移
			else
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
		}
		else if (bTap && !bHold)
		{
			ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
		}
		else if (bHold && gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Run_Attack); // ダッシュ攻撃に遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
		}
		else if (rtTap)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
		}
		else if (rtHold)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
		}
		else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			if (lockOnTargetPos != nullptr)
			{
				mode = MoveMode::Guarding_Walk;
				ChangeAnimationState(moveState);
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Walk_F); // ガード(歩き)に遷移
			}
			else if (moveLength < 0.1f && GamePad::BTN_LEFT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Guard); // ガードに遷移
			}
		}
		else if (moveLength > 0.5f)
		{
			mode = MoveMode::Jog;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の小走りへ遷移
			else
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
		}
		else if (moveLength > 0.1f && bHold)
		{
			ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
		}
		else if (moveLength > 0.1f && moveLength < 0.5f)
		{
			mode = MoveMode::Walk;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の歩きに遷移
			else
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
		}

		break;
	}

	// ===== ガード(歩き) =====
	case PlayerAnimationState::Guard_Walk_B:
	case PlayerAnimationState::Guard_Walk_BL:
	case PlayerAnimationState::Guard_Walk_BR:
	case PlayerAnimationState::Guard_Walk_F:
	case PlayerAnimationState::Guard_Walk_L:
	case PlayerAnimationState::Guard_Walk_R:
	{
		moveSpeed = 3.0f;

		if (jumpPressed)
		{
			ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
		}
		else if (moveLength > 0.1f && bTap)
		{
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(rollState); // ロックオン中の回避に遷移
			else
				ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
		}
		else if (bTap)
		{
			ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
		}
		else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
		{
			ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
		}
		else if (rtTap)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
		}
		else if (rtHold)
		{
			ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
		}
		else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
		{
			if (lockOnTargetPos != nullptr)
			{
				mode = MoveMode::Jog;
				ChangeAnimationState(moveState);
			}
			else if (moveLength > 0.5f)
			{
				ChangeAnimationState(PlayerAnimationState::Guard_Jog_F); // ガード(小走り)に遷移
			}
			else if (moveLength < 0.1f && GamePad::BTN_LEFT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Guard); // ガードに遷移
			}
		}
		else if (moveLength > 0.5f)
		{
			mode = MoveMode::Jog;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の小走りへ遷移
			else
				ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
		}
		else if (moveLength > 0.1f && bHold)
		{
			ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
		}
		else if (moveLength > 0.1f && moveLength < 0.5f)
		{
			mode = MoveMode::Walk;
			if (lockOnTargetPos != nullptr)
				ChangeAnimationState(moveState); // ロックオン中の歩きに遷移
			else
				ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
		}

		break;
	}

	// ===== ガード受け =====
	case PlayerAnimationState::Guard_Hit_01:
		break;
	case PlayerAnimationState::Guard_Hit_02:
		break;
	case PlayerAnimationState::Guard_Hit_03:
		break;

		// ===== バフ/戦技 =====
	case PlayerAnimationState::Buff:
		break;

		// ===== 装備 =====
	case PlayerAnimationState::Equip:
		break;

		// ===== 装備解除 =====
	case PlayerAnimationState::UnEquip:
		break;

		// ===== 回復 =====
	case PlayerAnimationState::Heal:
	{
		if (IsAnimationFinished()) ChangeAnimationState(PlayerAnimationState::Idle); // 待機に遷移
		else if (HealCoolDownTimer < 0.1f)
		{
			if (jumpPressed)
			{
				ChangeAnimationState(PlayerAnimationState::Jump_Start); // ジャンプへ遷移
			}
			else if (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER)
			{
				if (moveLength > 0.5f)
				{
					ChangeAnimationState(PlayerAnimationState::Guard_Jog_F); // ガード(小走り)に遷移
				}
				else if (moveLength > 0.1f && moveLength < 0.5f)
				{
					ChangeAnimationState(PlayerAnimationState::Guard_Walk_F); // ガード(歩き)に遷移
				}
				else
				{
					ChangeAnimationState(PlayerAnimationState::Guard); // ガードに遷移
				}
			}
			else if (moveLength > 0.5f)
			{
				mode = MoveMode::Jog;
				if (lockOnTargetPos != nullptr)
					ChangeAnimationState(moveState); // ロックオン中の小走りへ遷移
				else
					ChangeAnimationState(PlayerAnimationState::Jog_F); // 小走りへ遷移
			}
			else if (moveLength > 0.1f && bHold)
			{
				ChangeAnimationState(PlayerAnimationState::Run); // 走りへ遷移
			}
			else if (moveLength > 0.1f && moveLength < 0.5f)
			{
				mode = MoveMode::Walk;
				if (lockOnTargetPos != nullptr)
					ChangeAnimationState(moveState); // ロックオン中の歩きに遷移
				else
					ChangeAnimationState(PlayerAnimationState::Walk_F); // 歩きに遷移
			}
			else if (moveLength > 0.1f && bTap)
			{
				if (lockOnTargetPos != nullptr)
					ChangeAnimationState(rollState); // ロックオン中の回避に遷移
				else
					ChangeAnimationState(PlayerAnimationState::Roll_F); // 前回避に遷移
			}
			else if (bTap)
			{
				ChangeAnimationState(PlayerAnimationState::Dodge); // バックステップへ遷移
			}
			else if (gamePad.GetButtonDown() & GamePad::BTN_RIGHT_SHOULDER)
			{
				ChangeAnimationState(PlayerAnimationState::Attack_01); // 攻撃1に遷移
			}
			else if (rtTap)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack); // 強攻撃へ遷移
			}
			else if (rtHold)
			{
				ChangeAnimationState(PlayerAnimationState::Charge_Attack_Start); // 溜め攻撃へ遷移
			}
			else if (gamePad.GetButtonDown() & GamePad::BTN_X && Potion > 0)
			{
				ChangeAnimationState(PlayerAnimationState::Heal); // 回復に遷移
			}
		}

		break;
	}

		// ===== 大ダメージ =====
	case PlayerAnimationState::Hurt_Heavy_B:
		break;
	case PlayerAnimationState::Hurt_Heavy_B_Loop:
		break;
	case PlayerAnimationState::Hurt_Heavy_F:
		break;
	case PlayerAnimationState::Hurt_Heavy_F_Loop:
		break;
	case PlayerAnimationState::Hurt_End_B:
		break;
	case PlayerAnimationState::Hurt_End_F:
		break;

		// ===== 空中ダメージ =====
	case PlayerAnimationState::Hurt_In_Air_Start:
		break;
	case PlayerAnimationState::Hurt_In_Air_Loop:
		break;
	case PlayerAnimationState::Hurt_In_Air_End:
		break;

		// ===== 軽ダメージ =====
	case PlayerAnimationState::Hurt_Light_B:
		break;
	case PlayerAnimationState::Hurt_Light_F:
		break;
	case PlayerAnimationState::Hurt_Light_L:
		break;
	case PlayerAnimationState::Hurt_Light_R:
		break;

		// ===== アクション =====
	case PlayerAnimationState::Interaction:
		break;

		// ===== 拾う =====
	case PlayerAnimationState::PickUp_01:
		break;
	case PlayerAnimationState::PickUp_02:
		break;

		// ===== 座る =====
	case PlayerAnimationState::Sit_Start:
		break;
	case PlayerAnimationState::Sit_Loop:
		break;
	case PlayerAnimationState::Sit_End:
		break;

		// ===== 死亡 =====
	case PlayerAnimationState::Die:
		break;
	case PlayerAnimationState::Die_B:
		break;
	case PlayerAnimationState::Die_F:
		break;
	default:
		break;
	}
}

// 歩きのアニメションを決める関数
PlayerAnimationState Player::DetermineWalkState()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	float lx = gamePad.GetAxisLX(), ly = gamePad.GetAxisLY();

	//float power = sqrtf(lx * lx + ly * ly);
	//if (power < 0.1f) return AnimationState::Idle;

	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	//ベクトルの長さを計算
	float power = sqrtf((moveVec.x * moveVec.x) + (moveVec.z * moveVec.z));

	//入力がほぼなければIdleを返す
	if (power < 0.01f)return PlayerAnimationState::Idle;

	// キャラクターの前・右ベクトルを算出
	float charForwardX = sinf(angle.y), charForwardZ = cosf(angle.y);
	float charRightX = cosf(angle.y), charRightZ = -sinf(angle.y);

	// 内積により相対的な移動方向を判定
	float dotForward = moveVec.x * charForwardX + moveVec.z * charForwardZ;
	float dotRight = moveVec.x * charRightX + moveVec.z * charRightZ;

	//アークタンジェントで角度を算出
	float relativeAngle = atan2f(dotRight, dotForward);
	float degree = DirectX::XMConvertToDegrees(relativeAngle);
	if (degree < 0) degree += 360.0f;

	//デバッグ用変数更新
	int dirIndex = static_cast<int>((degree + 30.0f) / 60.0f) % 6;

	// 移動方向テーブル (通常 / ガード中)
	static const PlayerAnimationState moveAnimTable[4][6] = {
		{ PlayerAnimationState::Walk_F, PlayerAnimationState::Walk_R, PlayerAnimationState::Walk_BR, PlayerAnimationState::Walk_B, PlayerAnimationState::Walk_BL, PlayerAnimationState::Walk_L },
		{ PlayerAnimationState::Jog_F, PlayerAnimationState::Jog_R, PlayerAnimationState::Jog_BR, PlayerAnimationState::Jog_B, PlayerAnimationState::Jog_BL, PlayerAnimationState::Jog_L },
		{ PlayerAnimationState::Guard_Walk_F, PlayerAnimationState::Guard_Walk_BR, PlayerAnimationState::Guard_Walk_R, PlayerAnimationState::Guard_Walk_B, PlayerAnimationState::Guard_Walk_BL, PlayerAnimationState::Guard_Walk_L },
		{ PlayerAnimationState::Guard_Jog_F, PlayerAnimationState::Guard_Jog_R, PlayerAnimationState::Guard_Jog_BR, PlayerAnimationState::Guard_Jog_B, PlayerAnimationState::Guard_Jog_BL, PlayerAnimationState::Guard_Jog_L }
	};

	return moveAnimTable[static_cast<int>(mode)][dirIndex];
}

// 回避のアニメションを決める関数
PlayerAnimationState Player::DetermineRollState()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	float lx = gamePad.GetAxisLX(), ly = gamePad.GetAxisLY();

	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	//ベクトルの長さを計算
	float power = sqrtf((moveVec.x * moveVec.x) + (moveVec.z * moveVec.z));

	//入力がほぼなければIdleを返す
	if (power < 0.01f)return PlayerAnimationState::Idle;

	// キャラクターの前・右ベクトルを算出
	float charForwardX = sinf(angle.y), charForwardZ = cosf(angle.y);
	float charRightX = cosf(angle.y), charRightZ = -sinf(angle.y);

	// 内積により相対的な移動方向を判定
	float dotForward = moveVec.x * charForwardX + moveVec.z * charForwardZ;
	float dotRight = moveVec.x * charRightX + moveVec.z * charRightZ;

	//アークタンジェントで角度を算出
	float relativeAngle = atan2f(dotRight, dotForward);
	float degree = DirectX::XMConvertToDegrees(relativeAngle);
	if (degree < 0) degree += 360.0f;

	//デバッグ用変数更新
	debug_degree = degree;
	int dirIndex = static_cast<int>((degree + 36.0f) / 72.0f) % 5;
	debug_dirIndex = dirIndex;

	// 移動方向テーブル (通常 / ガード中)
	static const PlayerAnimationState moveAnimTable[5] = {
		PlayerAnimationState::Roll_F,
		PlayerAnimationState::Roll_R,
		PlayerAnimationState::Roll_BR,
		PlayerAnimationState::Roll_BL,
		PlayerAnimationState::Roll_L,
	};

	return moveAnimTable[dirIndex];
}

// 武器のアタッチメント処理
void Player::WeaponAttachment()
{
	const char* rightHandName = "hand_r";

	// 武器のローカル行列を計算する
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(weapon.scale.x, weapon.scale.y, weapon.scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(weapon.angle.x, weapon.angle.y, weapon.angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(weapon.position.x, weapon.position.y, weapon.position.z);
	DirectX::XMMATRIX weaponLocal = S * R * T;

	// キャラクターモデルから右手ノードを検索する
	for (const Model::Node& node : player->GetNodes())
	{
		if (strcmp(node.name.c_str(), rightHandName) == 0)
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