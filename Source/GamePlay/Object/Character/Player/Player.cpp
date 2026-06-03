#include "Player.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"
#include "System/Audio/Audio.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"
#include "GamePlay/Object/Character/Player/Magic/StraightMagic/StraightMagic.h"
#include "GamePlay/Object/Character/Player/Magic/HommingMagic/HommingMagic.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h" 

#include "imgui_node_editor.h"
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
	moveSpeed = 2.0f; // 移動速度
	maxMoveSpeed = 7.0f; // 最大移動速度

	health = 1400.0f; // 体力
	MaxHealth = 1400.0f; // 最大体力

	Stamina = 150; // 持久力
	MaxStamina = 150; // 最大持久力

	currentPoise = 100.0f;
	maxPoise = 100.0f;

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

	trail.Initialize();

	// アニメーションノードエディターの初期化
	{
		namespace fs = std::filesystem;
		std::string folderPath = "Data/Json/Player/AnimationState/";

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
			defaultGraph.Save("Data/Json/Player/AnimationState/BaseState.json");
			transitionGraphs.push_back(defaultGraph);
		}
	}

	// アニメーション設定
	LoadAnimationData("Data/Json/Player/AnimationState/BaseState.json");
	player->GetNodePoses(nodePoses);
	player->GetNodePoses(oldNodePoses);
	ChangeAnimationState("Idle");

}

// 攻撃とかの情報を初期化(Jsonファイルの初期化)
void Player::InitializeAttackData()
{
	// シーケンサーの初期化
	animSequence.SetModel(player);

	animSequence.SetConfigMap(&stateConfigs);

	// Jsonがあれば読み込む、無ければデフォルト値を設定
	std::ifstream file("Data/Json/Player/AttackData/AttackSequence.json");
	if (file.is_open())
	{
		file.close();
		animSequence.Load("Data/Json/Player/AttackData/AttackSequence.json");
	}
	else
	{
		//animSequence.attackData[PlayerAnimationState::Attack_01] = {
		//{ 40, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		//};
		//animSequence.attackData[PlayerAnimationState::Attack_02] = {
		//  { 55, 90, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		//};
		//animSequence.attackData[PlayerAnimationState::Charge_Attack] = {
		//  { 55, 82, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand },
		//  { 100, 127, u8"当たり判定2", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		//};
		//animSequence.attackData[PlayerAnimationState::Run_Attack] = {
		//  { 55, 100, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		//};
		//animSequence.attackData[PlayerAnimationState::Guard_Counter] = {
		//  { 55, 82, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		//};
		//animSequence.attackData[PlayerAnimationState::Jump_Attack] = {
		//  { 15, 42, u8"当たり判定1", 0xFF0000FF, TrackType::HitBox, HandType::RightHand }
		//};
	}
}

void Player::Finalize()
{
	sounds.clear();
}

// 更新処理
void Player::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 一番最初にジャンプ入力を確定させる
	jumpPressed = (gamePad.GetButtonDown() & GamePad::BTN_A)/* && CanJump()*/;

	if (runDisableTimer > 0.0f) {
		runDisableTimer -= elapsedTime;
		if (runDisableTimer < 0.0f)
		{
			runDisableTimer = 0.0f;
		}
	}

	// 無敵時間更新
	UpdateInvincibleTimer(elapsedTime);

	// スタミナ回復処理
	RecoveryStamina(elapsedTime);

	// 入力処理
	InputMove(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// 武器のアタッチメント処理
	WeaponAttachment();

	// 根本と剣先を求める関数
	CalculationRootAndTip();

	// 魔法発動処理
	MagicInput();

	// 魔法更新処理
	magicManager.Update(elapsedTime);

	// 状態遷移更新処理
	UpdateStateTransitions(elapsedTime);

	// アニメーション更新
	UpdateAnimation(elapsedTime);

	// サウンド再生
	UpdateSounds(this->currentState);

	// 回復
	if(!IsGuarding)// ガード中は回復しない
	Heal(elapsedTime);

	// エリア移動制限
	AreaRestriction();

	// モデル更新処理
	UpdateTransform();
	player->UpdateTransform(transform);
}

// 描画処理
void Player::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, player); 
	renderer->Draw(ShaderId::PBR, weapon.model);

	// 魔法描画
	magicManager.Render(rc);
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
			ImGui::Separator();
			if (IsStaminaEmpty)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
				ImGui::Text("Stamina: %f.0", Stamina);
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
				ImGui::Text("Stamina: %f.0", Stamina);
			}
			ImGui::PopStyleColor();
			ImGui::Separator();
			ImGui::Text("Damage: %f.0", lastDamage);
			ImGui::Separator();
			ImGui::Text("currentPoise: %f.0", currentPoise);
			ImGui::Separator();
			ImGui::Text("InvincibleTimer: %f.0", invincibleTimer);
			ImGui::Separator();

			ImGui::Text("Move Speed: %f.0", moveSpeed); // 移動速度
			ImGui::Text("Velocity: %.2f, %.2f, %.2f", velocity.x, velocity.y, velocity.z);
			ImGui::Separator();

			if (ImGui::Button(u8"ポーション回復"))
			{
				Potion = 14;
			}
			ImGui::Text("Potion: %d", Potion);
			ImGui::Separator();
			if (IsGuarding){
				ImGui::Text(u8"ガード中");
			}else{
				ImGui::Text(u8"ガードしてない");
			}

			if (IsAvoid) {
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

			ImGui::DragFloat3("Weapon HitOffset", &weapon.weaponHitOffset.x, 0.1f);
			ImGui::DragFloat3("Weapon AngleOffset", &weapon.weaponAngleOffset.x, 0.1f);
			ImGui::DragFloat("Weapon Collision Radius", &weapon.weaponRadius, 0.1f);
			ImGui::DragFloat("Weapon Collision Height", &weapon.weaponHeight, 0.1f);
		}

		// トレイル
		if (ImGui::CollapsingHeader("Weapon Trail", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat3("Root Offset", &rootOffset.x, 0.01f);
			ImGui::DragFloat3("Tip Offset", &tipOffset.x, 0.01f);
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
	}
	ImGui::End();

	ImGui::Begin("Animation Transition Editor");
	std::string clickedNode = transitionEditor.Draw(transitionGraphs);
	ImGui::End();

	if (clickedNode != "")
	{
		animSequence.currentState = clickedNode;

		ChangeAnimationState(clickedNode);

		ImGui::SetWindowFocus("Player Attack Sequencer");
	}

	PlayerAnimationSequencer();
}

// デバックプリミティブ描画
void Player::RenderDebugPrimitive(ShapeRenderer* renderer, bool showWeaponHitBox)
{
	// プレイヤーの当たり判定
	if(!IsAvoid)
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

	// 魔法の当たり判定描画
	magicManager.RenderDebugPrimitive(renderer);
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

// 魔法射出
void Player::MagicInput()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	
	// 直進弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_Y)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);
		// 発射位置（仮でプレイヤーの腰当たり）
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + 1.0f;// 仮で腰当たりに配置
		pos.z = position.z;
		// 発射
		StraightMagic* magic = new StraightMagic(&magicManager);
		magic->Launch(dir, pos);
	}

	// 追尾弾丸発射
	if (gamePad.GetButtonDown() & GamePad::BTN_X)
	{
		// 前方向
		DirectX::XMFLOAT3 dir;
		dir.x = sinf(angle.y);
		dir.y = 0.0f;
		dir.z = cosf(angle.y);
		// 発射位置（仮でプレイヤーの腰当たり）
		DirectX::XMFLOAT3 pos;
		pos.x = position.x;
		pos.y = position.y + 1.0f;// 仮で腰当たりに配置
		pos.z = position.z;
		//ターゲットを追尾する
		DirectX::XMFLOAT3 target;
		target.x = pos.x + dir.x * 1000.0f;
		target.y = pos.y + dir.y * 1000.0f;
		target.z = pos.z + dir.z * 1000.0f;

		//一番近くの敵をターゲットにする
		float dist = FLT_MAX;
		// 敵との距離判定
		DirectX::XMVECTOR Postion = DirectX::XMLoadFloat3(&position);
		DirectX::XMVECTOR EPosition = DirectX::XMLoadFloat3(&enemy->GetPosition());
		DirectX::XMVECTOR Vec = DirectX::XMVectorSubtract(Postion, EPosition);
		DirectX::XMVECTOR Length = DirectX::XMVector3LengthSq(Vec);
		float d;
		DirectX::XMStoreFloat(&d, Length);
		if (d < dist)
		{
			dist = d;
			target = enemy->GetPosition();
			target.y += enemy->GetHeight() * 0.5f;
		}

		// 発射
		HommingMagic* magic = new HommingMagic(&magicManager);
		magic->Launch(dir, pos, target);
	}
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

// スタミナ計算処理
void Player::calculationStamina(float stamina)
{
	Stamina -= stamina;
	if (Stamina <= 0.0f)
	{
		Stamina = 0.0f;
		IsStaminaEmpty = true; // スタミナが空になったと通知する
		runDisableTimer = 3.0f;
	}
	staminaCooldownTimer = 1.5f; // 1.5秒経ったらスタミナを回復させる
}

// スタミナ回復処理
void Player::RecoveryStamina(float elapsedTime)
{
	if (staminaCooldownTimer > 0.0f)
	{
		staminaCooldownTimer -= elapsedTime;
		if (staminaCooldownTimer < 0.0f)
		{
			staminaCooldownTimer = 0.0f;
		}
	}
	else
	{
		IsStaminaEmpty = false;
		Stamina += elapsedTime * 20.0f;
		if (Stamina >= MaxStamina) Stamina = MaxStamina;
	}
}

// ロックオン対象の位置を取得
void Player::SetLockOnTargetPosition(const DirectX::XMFLOAT3* pos) 
{
	lockOnTargetPos = pos;
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
	std::string path = "Data/Json/Player/AnimationState/" + name + ".json";
	transitionGraphs.back().Save(path);
}

// シーケンサーを描画する
void Player::PlayerAnimationSequencer()
{
	ImGui::Begin("Player Attack Sequencer");
	auto& AnimSequence = GetAnimSequence();

	float totalSec = AnimSequence.GetAnimationLength(AnimSequence.currentState);
	ImGui::Text(u8"総秒数: %.2f秒  (バーの数値 ÷ 100 = 秒)", totalSec);
	auto& tracks = AnimSequence.CurrentTracks();

	// 今選ばれているステート名をわかりやすく表示する（空っぽなら "None"）
	std::string previewName = AnimSequence.currentState.empty() ? "None" : AnimSequence.currentState;

	if (ImGui::BeginCombo(u8"編集中のアニメ", previewName.c_str()))
	{
		// 自身の stateConfigs (ノードエディタのデータ) から一覧を作る
		for (auto& [stateName, config] : stateConfigs)
		{
			bool is_selected = (AnimSequence.currentState == stateName);

			// リストにステート名を表示
			if (ImGui::Selectable(stateName.c_str(), is_selected))
			{
				AnimSequence.currentState = stateName;
				ChangeAnimationState(stateName);
			}

			// 選択中のものをフォーカスする
			if (is_selected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::Separator(); 

	if (selectedEntry >= 0)
	{
		//選択中のトラックを参照
		auto& t = tracks[selectedEntry];

		char inputlabelName[256];
		// t.labelの中身をinputlabelNameにコピーする
		strncpy_s(inputlabelName, t.label.c_str(), sizeof(inputlabelName));
		if (ImGui::InputText(u8"ラベル名", inputlabelName, IM_ARRAYSIZE(inputlabelName)))
		{
			t.label = inputlabelName;
		}
		ImGui::Separator();

		// シーケンサーがunsigned int型を要求しているためfloat[4]型に変換
		ImVec4 col = ImGui::ColorConvertU32ToFloat4(t.color);
		if (ImGui::ColorEdit4(u8"ラベルの色", &col.x))
		{
			// 編集してるときにunsigned intに戻して保存
			t.color = ImGui::GetColorU32(col);
		}

		ImGui::Separator();

		switch (t.type)
		{
		case TrackType::HitBox:
			ImGui::Begin(u8"当たり判定設定");
			{
				ImGui::Text(u8"選択中: %.2f秒 〜 %.2f秒",
					t.GetStartSeconds(), t.GetEndSeconds());

				// TrackType の変更
				const char* trackTypeItems[] = { "HitBox", "Effect", "Sound" };
				int trackTypeIndex = (int)t.type;
				if (ImGui::Combo(u8"タイプ", &trackTypeIndex, trackTypeItems, sizeof(trackTypeItems)))
				{
					t.type = (TrackType)trackTypeIndex;
				}

				// HandType の変更（HitBox のときだけ表示）
				if (t.type == TrackType::HitBox)
				{
					const char* handItems[] = { "None", "RightHand", "LeftHand", "Both", "Body" };
					int handIndex = (int)t.hand;
					if (ImGui::Combo(u8"手", &handIndex, handItems, sizeof(handItems)))
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
			ImGui::End();
			break;
		case TrackType::Effect:
			break;
		case TrackType::Sound:
			ImGui::Begin(u8"サウンド設定");
			{
				char soundbuf[256] = "";
				strncpy_s(soundbuf, t.soundName.c_str(), sizeof(soundbuf));
				if (ImGui::InputText(u8"SE名を入力してください", soundbuf, sizeof(soundbuf)))
				{
					t.soundName = soundbuf;
				}

				if (ImGui::Button(u8"視聴する"))
				{
					GetOrLoadSound(t.soundName);
					sounds[t.soundName]->Play(false);
				}
			}
			ImGui::End();
			break;
		}

	}
	// 選択中のステートにトラックを追加するボタン
	if (ImGui::Button(u8"+ 追加"))
	{
		ImGui::OpenPopup("AddTrack");
	}

	if (ImGui::BeginPopup("AddTrack"))
	{
		ImGui::Text(u8"追加するトラックタイプを選択してください");
		ImGui::Separator();

		if (ImGui::Selectable(u8"当たり判定を追加"))
		{			
			std::string label = u8"HitBox" + std::to_string(tracks.size());
			tracks.push_back({ 0, 50, label, 0xFF0000FF, TrackType::HitBox, HandType::RightHand });
		}

		if (ImGui::Selectable(u8"サウンド追加"))
		{
			std::string label = u8"Sound" + std::to_string(tracks.size());
			tracks.push_back({ 0, 50, label, 0xFF00FFFF, TrackType::Sound });
		}

		ImGui::EndPopup();
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
		AnimSequence.Save("Data/Json/Player/AttackData/AttackSequence.json");
	ImGui::SameLine();
	if (ImGui::Button(u8"読み込み"))
		AnimSequence.Load("Data/Json/Player/AttackData/AttackSequence.json");

	if (AnimSequence.currentState == GetCurrentState())
	{
		currentFrame = (int)(GetCurrentAnimationSeconds() * 100);
	}

	ImSequencer::Sequencer(
		&AnimSequence,
		&currentFrame,
		&sequencerExpanded,
		&selectedEntry,
		&firstFrame,
		ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME
	);

	ImGui::End();
}

// サウンドを流す
void Player::UpdateSounds(const std::string& state)
{
	auto& AnimSequence = GetAnimSequence();
	float now = GetCurrentAnimationSeconds();

	auto& tracks = AnimSequence.attackData[state];

	for (auto& track : tracks)
	{
		if (track.type != TrackType::Sound) continue; // 音トラックのみ通す
		if (track.soundPlayed) continue;

		if (now >= track.GetStartSeconds()) {
			AudioSource* se = GetOrLoadSound(track.soundName);
			if (se) {
				se->Play(false); // SEやからループ無し
			}
			track.soundPlayed = true; // フラグを立てる
		}
	}
}

//着地したときに呼ばれる
void Player::OnLanding()
{
	jumpCount = 0;
}

// アニメーションのコールバック関数
void Player::OnStateChanged(const std::string& oldState, const std::string& newState)
{
	// 新しいステートのトラックフラグをリセット
	for (auto& track : animSequence.attackData[newState])
	{
		track.soundPlayed = false;
	}
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

	if (currentState == "Run")
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
//bool Player::CanJump() const
//{
//	//// Jump_End 中はジャンプ不可（着地の瞬間のフレームのズレ対策）
//	//if (currentState == PlayerAnimationState::Jump_End)
//	//{
//	//	return IsAnimationOutTimeRange(0.238f);
//	//}
//
//	//if (currentState == PlayerAnimationState::Charge_Attack_Start) return false;
//	//// 溜め攻撃の開始直後などは絶対にジャンプ不可
//	//if (currentState == PlayerAnimationState::Charge_Attack_Start) return false;
//
//	//// 通常攻撃の場合
//	//if (currentState == PlayerAnimationState::Attack_01) 
//	//{
//	//	return IsAnimationOutTimeRange(0.82f);
//	//}
//	//else if (currentState == PlayerAnimationState::Attack_02)
//	//{
//	//	return IsAnimationOutTimeRange(0.82f);
//	//}
//	//else if (currentState == PlayerAnimationState::Charge_Attack)
//	//{
//	//	return IsAnimationOutTimeRange(1.2f);
//
//	//}
//	//else if (currentState == PlayerAnimationState::Run_Attack)
//	//{
//	//	return IsAnimationOutTimeRange(1.248f);
//
//	//}
//	//else if (currentState == PlayerAnimationState::Jump_Attack)
//	//{
//	//	return IsAnimationOutTimeRange(0.986);
//
//	//}
//
//	//return true;
//}

// 状態遷移更新処理
void Player::UpdateStateTransitions(float elapsedTime)
{
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	float moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.z * moveVec.z);
	GamePad& gamePad = Input::Instance().GetGamePad();

	std::string moveState = DetermineWalkState(); // 歩きの遷移条件を取得
	std::string rollState = DetermineRollState(); // 回避の遷移条件を取得

	bool bHold = bButtonHoldTime >= RUN_THRESHOLD; // 長押し判定

	// 短押し判定：離した瞬間 かつ 長押しじゃなかった
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

	IsGuarding = (gamePad.GetButton() & GamePad::BTN_LEFT_SHOULDER);

	HaveStamina = (Stamina > 0.1f);

	bool canRunTrigger = (runDisableTimer <= 0.0f); // 3秒ペナルティが終わっているか

	UpdateStateBehavior();

	TransitionContext ctx;
	ctx.position = GetPosition();
	ctx.animSeconds = GetCurrentAnimationSeconds();
	ctx.animLength = GetCurrentAnimationLength();
	ctx.moveLength = moveLength;
	ctx.buttonDown = gamePad.GetButtonDown();
	ctx.buttonHeld = gamePad.GetButton();
	ctx.buttonUp = gamePad.GetButtonUp();
	ctx.bHold = bButtonHoldTime >= RUN_THRESHOLD;
	ctx.bTap = (gamePad.GetButtonUp() & GamePad::BTN_B) && !ctx.bHold;
	ctx.rbTap = (gamePad.GetButtonUp() & GamePad::BTN_RIGHT_SHOULDER) && !ctx.bHold;
	ctx.rtHold = rtButtonHoldTime >= ATTACK_THRESHOLD;
	ctx.rtTap = (gamePad.GetButtonUp() & GamePad::BTN_RIGHT_TRIGGER) && !ctx.rtHold;
	ctx.jumpPressed = jumpPressed;
	ctx.haveStamina = HaveStamina;
	ctx.isStaminaEmpty = IsStaminaEmpty;
	ctx.canRunTrigger = canRunTrigger;
	ctx.isLockOn = (lockOnTargetPos != nullptr);
	ctx.havePotion = (Potion > 0);
	ctx.healCooldownReady = (HealCoolDownTimer < 0.1f);
	ctx.isGuarding = IsGuarding;

	// グラフで評価して遷移する

	for (auto& graph : transitionGraphs)
	{
		std::string nextState = graph.EvaluateTransitions(currentState, ctx);
		if (nextState != currentState)
		{
			// 遷移時のアクション
			const AnimationTransition* trans = graph.GetTransition(currentState, nextState);
			if (trans)
			{
				for (auto& action : trans->actions)
				{
					switch (action.type)
					{
					case TransitionActionType::MoveSpeed:
						moveSpeed = action.value;
						break;

					case TransitionActionType::TurnSpeed:
						turnSpeed = DirectX::XMConvertToRadians(action.value);
						break;

					case TransitionActionType::ConsumeStamina:
						calculationStamina(action.value);
						break;

					case TransitionActionType::SetIsAvoid:
						IsAvoid = (action.value != 0.0f);
						break;

					case TransitionActionType::SetAnimationSpeed:
						SetBaseSpeed(action.value);
						break;
					}
				}
			}
			ChangeAnimationState(nextState);
		}
	}
}

// 遷移以外の細かい条件
void Player::UpdateStateBehavior()
{
	if (currentState == "Run")
	{
		calculationStamina(0.1f);
	}
	else if (currentState == "Charge_Attack_Start")
	{
		calculationStamina(0.5f);
		AnimationLerp(0.188f, 0.583f, 0.4f);
	}
}

// 歩きのアニメーションを決める関数
std::string Player::DetermineWalkState()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	float lx = gamePad.GetAxisLX(), ly = gamePad.GetAxisLY();

	//float power = sqrtf(lx * lx + ly * ly);
	//if (power < 0.1f) return AnimationState::Idle;

	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	//ベクトルの長さを計算
	float power = sqrtf((moveVec.x * moveVec.x) + (moveVec.z * moveVec.z));

	//入力がほぼなければIdleを返す
	if (power < 0.01f)return "Idle";

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
	static const std::string moveAnimTable[4][6] = {
	{ "Walk_F", "Walk_R", "Walk_BR", "Walk_B", "Walk_BL", "Walk_L" },
	{ "Jog_F", "Jog_R", "Jog_BR", "Jog_B", "Jog_BL", "Jog_L" },
	{ "Guard_Walk_F", "Guard_Walk_BR", "Guard_Walk_R", "Guard_Walk_B", "Guard_Walk_BL", "Guard_Walk_L" },
	{ "Guard_Jog_F", "Guard_Jog_R", "Guard_Jog_BR", "Guard_Jog_B", "Guard_Jog_BL", "Guard_Jog_L" } };

	return moveAnimTable[static_cast<int>(mode)][dirIndex];
}

// 回避のアニメーションを決める関数
std::string Player::DetermineRollState()
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	float lx = gamePad.GetAxisLX(), ly = gamePad.GetAxisLY();

	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	//ベクトルの長さを計算
	float power = sqrtf((moveVec.x * moveVec.x) + (moveVec.z * moveVec.z));

	//入力がほぼなければIdleを返す
	if (power < 0.01f)return "Idle";

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
	static const std::string moveAnimTable[5] = {
		"Roll_F",
		"Roll_R",
		"Roll_BR",
		"Roll_BL",
		"Roll_L",
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

// 剣先と根本を求める関数
void Player::CalculationRootAndTip()
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
