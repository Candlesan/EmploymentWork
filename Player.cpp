#include "Player.h"
#include "Graphics.h"
#include "Input.h"
#include <imgui.h>
#include <cmath>

//ログ出力用
#include <string>
#include <sstream>
#include <Windows.h>

// コンストラクタ
Player::Player()
{
	OutputDebugStringA("[GetActor OK before Start]\n");
	if (!player) {
		OutputDebugStringA("[Model is NULL!]\n");
	}
	else {
		char buf[256];
		sprintf_s(buf, "[Model OK] Has %zu animations\n", player->GetAnimations().size());
		OutputDebugStringA(buf);
		// 全アニメーション名を出力
		for (size_t i = 0; i < player->GetAnimations().size(); ++i) {
			sprintf_s(buf, "  Animation[%zu]: '%s'\n", i, player->GetAnimations()[i].name.c_str());
			OutputDebugStringA(buf);
		}
	}

	// デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// モデル初期化
	player = std::make_shared<Model>(device, "Data/Model/Player/Map_Robot3.gltf");

	// アニメーション用に姿勢を初期化
	player->GetNodePoses(nodePoses);
	player->GetNodePoses(oldNodePoses);
	state = State::Idle;
}

// 更新処理
void Player::Update(float elapsedTime)
{
	// 移動入力処理
	InputMove(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// アニメーション更新
	UpdateAnimations(elapsedTime);

	//攻撃入力処理
	Attack();

	// オブジェクト行列を更新
	UpdateTransform();

	// モデル行列更新
	player->UpdateTransform(transform);
}

// 描画処理
void Player::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::PBR, player);
}

// GUI描画
void Player::DrawGui()
{
	ImGui::Begin("Player");

	//	トランスフォーム
	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		//位置
		ImGui::InputFloat3("Position", &position.x);
		//回転
		DirectX::XMFLOAT3 a;
		a.x = DirectX::XMConvertToDegrees(angle.x);
		a.y = DirectX::XMConvertToDegrees(angle.y);
		a.z = DirectX::XMConvertToDegrees(angle.z);
		ImGui::InputFloat3("Angle", &a.x);
		a.y = DirectX::XMConvertToRadians(angle.y);
		a.z = DirectX::XMConvertToRadians(angle.z);
		a.x = DirectX::XMConvertToRadians(angle.x);
		//スケール
		ImGui::InputFloat3("Scale", &scale.x);
	}

	if (ImGui::CollapsingHeader("Movement", ImGuiTreeNodeFlags_DefaultOpen))
	{
		GamePad& gamePad = Input::Instance().GetGamePad();
		DirectX::XMFLOAT3 moveVec = GetMoveVec();
		float moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.y * moveVec.y + moveVec.z * moveVec.z);

		ImGui::DragFloat("move", &moveSpeed, 0.01f);
		ImGui::Text("moveLength %f", moveLength);

		ImGui::DragFloat("turn", &turnSpeed, 0.01f);
	}

	if (ImGui::CollapsingHeader("Attack Debug", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextWrapped("Info: %s", debug_attack_log.c_str());
	}

	ImGui::End();
}

// 移動入力処理
void Player::InputMove(float elapsedTime)
{
	// 進行ベクトル取得
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	if (useRootMotion || useRootMotionEx)
	{
		Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
		return;
	}

	// 旋回処理
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);

	// 移動処理
	Move(moveVec.x, moveVec.z, moveSpeed);
}

//スティック入力またはマウスの角度から8方向のいずれかを判断して攻撃
void Player::Attack()
{
	//マウスの移動力を取得
	ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;

	//マウスの移動力が小さい場合は処理をしない
	const float threshold = 0.5f;
	if (fabsf(mouse_delta.x) < threshold && fabsf(mouse_delta.y) < threshold)
	{
		return;
	}

	//入力角度を計算
	float angle = atan2(-mouse_delta.y, mouse_delta.x);

	//角度を8方向にスナップさせる
	const float step = DirectX::XM_PI / 4.0f;//45度
	float snapped_angle = roundf(angle / step) * step;

	//スナップした角度からローカル方向ベクトルを算出
	float local_x = cosf(snapped_angle);
	float local_z = sinf(snapped_angle);

	//カメラの向きに合わせてワールド座標に変換
	Camera& camera = CameraManager::Instance().GetMainCamera();
	DirectX::XMFLOAT3 camera_right = camera.GetRight();
	DirectX::XMFLOAT3 camera_front = camera.GetFront();
	DirectX::XMVECTOR CameraRightVec = DirectX::XMLoadFloat3(&camera_right);
	DirectX::XMVECTOR CameraFrontVec = DirectX::XMLoadFloat3(&camera_front);

	//水平攻撃にするためY成分を削除して正規化
	CameraRightVec = DirectX::XMVectorSetY(CameraRightVec, 0.0f);
	CameraFrontVec = DirectX::XMVectorSetY(CameraFrontVec, 0.0f);
	CameraRightVec = DirectX::XMVector3Normalize(CameraRightVec);
	CameraFrontVec = DirectX::XMVector3Normalize(CameraFrontVec);

	//ローカル方向をカメラ基準のワールド方向に合成
	DirectX::XMVECTOR AttackDirVec = DirectX::XMVectorAdd(
		DirectX::XMVectorScale(CameraRightVec, local_x),
		DirectX::XMVectorScale(CameraFrontVec, local_z)
	);

	//正規化
	AttackDirVec = DirectX::XMVector3Normalize(AttackDirVec);

	//結果を格納
	DirectX::XMFLOAT3 attack_dir;
	DirectX::XMStoreFloat3(&attack_dir, AttackDirVec);
	DirectX::XMStoreFloat3(&attack_dir, AttackDirVec);

	//ログ出力
	std::stringstream ss;
	ss << "Attack 8-Dir: X=" << attack_dir.x
		<< " Z=" << attack_dir.z
		<< " (Angle: " << DirectX::XMConvertToDegrees(snapped_angle) << " deg)\n";

	OutputDebugStringA(ss.str().c_str());

	debug_attack_log = ss.str();
}

DirectX::XMFLOAT3 Player::GetMoveVec()
{
	//入力情報を取得
	GamePad& gamePad = Input::Instance().GetGamePad();
	float ax = gamePad.GetAxisLX();
	float ay = gamePad.GetAxisLY();

	//カメラ方向とスティックの入力値によって進行方向を計算する
	Camera& camera = CameraManager::Instance().GetMainCamera();
	const DirectX::XMFLOAT3& cameraRight = camera.GetRight();
	const DirectX::XMFLOAT3& cameraFront = camera.GetFront();

	//移動ベクトルはXZ平面に水平なベクトルにする

	//カメラ右方向ベクトルをXZ単位ベクトルに変換
	float cameraRightX = cameraRight.x;
	float cameraRightZ = cameraRight.z;
	float cameraRightLength = sqrtf(cameraRightX * cameraRightX + cameraRightZ * cameraRightZ);
	if (cameraRightLength > 0.0f)
	{
		//単位ベクトル化 
		cameraRightX /= cameraRightLength;
		cameraRightZ /= cameraRightLength;
	}

	//カメラ前方向ベクトルをXZ単位ベクトルに変換
	float cameraFrontX = cameraFront.x;
	float cameraFrontZ = cameraFront.z;
	float cameraFrontLength = sqrtf(cameraFrontX * cameraFrontX + cameraFrontZ * cameraFrontZ);
	if (cameraFrontLength > 0.0f)
	{
		//単位ベクトル化 
		cameraFrontX /= cameraFrontLength;
		cameraFrontZ /= cameraFrontLength;
	}

	//スティックの水平入力値をカメラ右方向に反映し、
	//スティックの垂直入力値をカメラ前方向に反映し、
	//進行ベクトルを計算する
	DirectX::XMFLOAT3 vec;
	vec.x = (cameraRightX * ax) + (cameraFrontX * ay);
	vec.z = (cameraRightZ * ax) + (cameraFrontZ * ay);
	//Y軸方向には移動しない
	vec.y = 0.0f;

	return vec;
}

// アニメーション更新処理
void Player::UpdateAnimations(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	float moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.y * moveVec.y + moveVec.z * moveVec.z);

	int newAnimationIndex = animationIndex;

	switch (state)
	{
	case Player::State::Idle:
		animationLoop = true;
		useRootMotion = false;
		newAnimationIndex = player->GetAnimationIndex("Idle");

		if (moveLength > 0.01f)
		{
			state = State::Walk;
		}

		break;
	case Player::State::Walk:
		animationLoop = true;
		useRootMotion = false;
		newAnimationIndex = player->GetAnimationIndex("Anim_MM_Run_Fwd_0");

		if (moveLength < 0.01f)
		{
			state = State::Idle;
		}

		if (moveLength > 0.5f)
		{
			state = State::Run;
		}

		break;
	case Player::State::Run:
		animationLoop = true;
		useRootMotionEx = true;
		// 切り替えの瞬間に新しいアニメーションの開始地点へ座標が引っ張られるかも
		// 知らんからルートモーションを使う時はブレンドをOFFにしといて
		isBlending = false; 
							
		newAnimationIndex = player->GetAnimationIndex("Run");

		if (moveLength < 0.01f)
		{
			state = State::Idle;
		}

		break;
	}

	// アニメーション切り替え判定
	if (animationIndex != newAnimationIndex)
	{
		// 前のアニメーションの姿勢を保存
		oldNodePoses = nodePoses;

		// 新しいアニメーションに切り替え
		animationIndex = newAnimationIndex;
		animationSeconds = 0.0f;
		oldAnimationSeconds = 0.0f;

		// 補間開始
		animationBlendSeconds = 0.0f;
		isBlending = true;

	}

	// 補間時間更新
	if (isBlending)
	{
		animationBlendSeconds += elapsedTime;
		if (animationBlendSeconds >= animationBlendSecondsLength)
		{
			isBlending = false;
			animationBlendSeconds = animationBlendSecondsLength;
		}
	}

	if (animationIndex >= 0)
	{
		const Model::Animation& animation = player->GetAnimations().at(animationIndex);

		// 現在のアニメーションの姿勢を取得
		std::vector<Model::NodePose> currentNodePoses;
		player->ComputeAnimation(animationIndex, animationSeconds, currentNodePoses);

		// 補間処理
		if (isBlending && animationBlendSeconds < animationBlendSecondsLength)
		{
			// 補間率を計算
			float blendRate = animationBlendSeconds / animationBlendSecondsLength;

			// 前のアニメーションと現在のアニメーションを補間
			player->BlendAnimations(oldNodePoses, currentNodePoses, blendRate, nodePoses);
		}
		else
		{
			// 補間なしで現在のアニメーション姿勢を使用
			nodePoses = currentNodePoses;
		}

		if (useRootMotion) // EX課題ではない方のルートモーション（してほしいときは川上に言ってくれ）
		{
			// ルートモーションノード番号取得
			const int rootMotionNodeIndex = player->GetNodeIndex("pelvis"); // モデルに合わせて変えてくれ

			// 初回、前回、今回のルートモーションの姿勢を取得
			Model::NodePose beginPose, oldPose, newPose;
			player->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
			player->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
			player->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

			//ローカル移動値を算出
			DirectX::XMFLOAT3 localTranslation;
			if (oldAnimationSeconds > animationSeconds)
			{
				// ループ時処理
				Model::NodePose endPose;
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);

				// ローカル移動値を算出
				localTranslation.x = (endPose.position.x - oldPose.position.x) +
					(newPose.position.x - beginPose.position.x);
				localTranslation.y = (endPose.position.y - oldPose.position.y) +
					(newPose.position.y - beginPose.position.y);
				localTranslation.z = (endPose.position.z - oldPose.position.z) +
					(newPose.position.z - beginPose.position.z);
			}
			else
			{
				localTranslation.x = newPose.position.x - oldPose.position.x;
				localTranslation.y = newPose.position.y - oldPose.position.y;
				localTranslation.z = newPose.position.z - oldPose.position.z;
			}

			// グローバル移動値を算出
			Model::Node& rootMotionNode = player->GetNodes().at(rootMotionNodeIndex);
			DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);
			DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&rootMotionNode.parent->globalTransform);
			DirectX::XMVECTOR GlobalTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, ParentGlobalTransform);

			if (bakeTranslationY)
			{
				// Y成分の移動値を抜く 
				GlobalTranslation = DirectX::XMVectorSetY(GlobalTranslation, 0);

				// 今回の姿勢のグローバル位置を算出 
				DirectX::XMVECTOR LocalPos = DirectX::XMLoadFloat3(&newPose.position);
				DirectX::XMVECTOR currentGlobalPos = DirectX::XMVector3Transform(LocalPos, ParentGlobalTransform);

				// XZ成分を削除 
				currentGlobalPos = DirectX::XMVectorSetX(currentGlobalPos, 0);
				currentGlobalPos = DirectX::XMVectorSetZ(currentGlobalPos, 0);

				// ローカル空間変換 
				DirectX::XMMATRIX invGlobalTransform = DirectX::XMMatrixInverse(nullptr, ParentGlobalTransform);
				LocalPos = DirectX::XMVector3Transform(currentGlobalPos, invGlobalTransform);

				// ルートモーションノードの位置を設定 
				DirectX::XMStoreFloat3(&nodePoses[rootMotionNodeIndex].position, LocalPos);
			}
			else
			{
				//ルートモーションノードを初回の姿勢にする
				nodePoses[rootMotionNodeIndex].position = beginPose.position;
			}

			//ワールド移動値を算出
			DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&worldTransform);
			DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(GlobalTranslation, WorldTransform);
			DirectX::XMFLOAT3 worldTranslation;
			DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);

			//移動値を更新
			position.x += worldTranslation.x;
			position.y += worldTranslation.y;
			position.z += worldTranslation.z;
		}
		else if (useRootMotionEx)
		{
			// ルートモーションノード番号取得（モデルに応じて適切なノード名を指定）
			const int rootMotionNodeIndex = player->GetNodeIndex("pelvis");
			if (rootMotionNodeIndex >= 0)
			{
				// 初回、前回、今回のルートモーションの姿勢を取得
				Model::NodePose beginPose, oldPose, newPose;
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

				// ローカル移動値を算出
				DirectX::XMFLOAT3 localTranslation;
				if (oldAnimationSeconds > animationSeconds)
				{
					// ループ時処理
					Model::NodePose endPose;
					player->ComputeAnimation(animationIndex, rootMotionNodeIndex, animation.secondsLength, endPose);
					// ローカル移動値を算出
					localTranslation.x = (endPose.position.x - oldPose.position.x) +
						(newPose.position.x - beginPose.position.x);
					localTranslation.y = (endPose.position.y - oldPose.position.y) +
						(newPose.position.y - beginPose.position.y);
					localTranslation.z = (endPose.position.z - oldPose.position.z) +
						(newPose.position.z - beginPose.position.z);
				}
				else
				{
					localTranslation.x = newPose.position.x - oldPose.position.x;
					localTranslation.y = newPose.position.y - oldPose.position.y;
					localTranslation.z = newPose.position.z - oldPose.position.z;
				}

				// グローバル移動値を算出
				Model::Node& rootMotionNode = player->GetNodes().at(rootMotionNodeIndex);
				DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);
				DirectX::XMMATRIX ParentGlobalTransform = DirectX::XMLoadFloat4x4(&rootMotionNode.parent->globalTransform);
				DirectX::XMVECTOR GlobalTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, ParentGlobalTransform);

				if (bakeTranslationY)
				{
					// Y成分の移動値を抜く 
					GlobalTranslation = DirectX::XMVectorSetY(GlobalTranslation, 0);

					// 今回の姿勢のグローバル位置を算出 
					DirectX::XMVECTOR LocalPos = DirectX::XMLoadFloat3(&newPose.position);
					DirectX::XMVECTOR currentGlobalPos = DirectX::XMVector3Transform(LocalPos, ParentGlobalTransform);

					// XZ成分を削除 
					currentGlobalPos = DirectX::XMVectorSetX(currentGlobalPos, 0);
					currentGlobalPos = DirectX::XMVectorSetZ(currentGlobalPos, 0);

					// ローカル空間変換 
					DirectX::XMMATRIX invGlobalTransform = DirectX::XMMatrixInverse(nullptr, ParentGlobalTransform);
					LocalPos = DirectX::XMVector3Transform(currentGlobalPos, invGlobalTransform);

					// ルートモーションノードの位置を設定 
					DirectX::XMStoreFloat3(&nodePoses[rootMotionNodeIndex].position, LocalPos);
				}
				else
				{
					//ルートモーションノードを初回の姿勢にする
					nodePoses[rootMotionNodeIndex].position = beginPose.position;
				}

				// ワールド移動値を算出（キャラクターの位置に足す）
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&transform);
				DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(GlobalTranslation, WorldTransform);
				DirectX::XMFLOAT3 worldTranslation;
				DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);

				//移動値を更新
				position.x += worldTranslation.x;
				position.y += worldTranslation.y;
				position.z += worldTranslation.z;
			}
		}

		// アニメーション時間更新
		oldAnimationSeconds = animationSeconds;
		animationSeconds += elapsedTime;
		if (animationSeconds > animation.secondsLength)
		{
			if (animationLoop)
			{
				animationSeconds -= animation.secondsLength;
			}
			else
			{
				animationSeconds = animation.secondsLength;
			}
		}

		//姿勢更新
		player->SetNodePoses(nodePoses);
	}
}

// アニメーションが終了したか
bool Player::FinshedAnimation()
{
	if (animationIndex > -1)
	{
		const Model::Animation& animation = player->GetAnimations().at(animationIndex);

		if (animationSeconds >= animation.secondsLength)
		{
			return true;
		}
	}
	return false;
}
