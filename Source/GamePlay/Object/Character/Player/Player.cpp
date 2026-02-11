#include "Player.h"

// システム
#include "System/Graphic/Graphics.h"
#include "System/Core/Input/Input.h"

// ゲームオブジェクト
#include "GamePlay/Object/Camera/Camera.h"

#include <imgui.h>

// 初期化
void Player::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// プレイヤーモデル読み込み
	player = std::make_shared<Model>(device, "Data/Model/Player/Map_Robot3.gltf");

	// 武器モデル読み込み
	weapon.model = std::make_shared<Model>(device, "Data/Model/Weapon/SM_GreatSword.gltf");

	 // プレイヤーパラメーター初期化
	moveSpeed = 2.0f;

	// 当たり判定パラメーター初期化
	weight = 0.5f;
	height = 1.0f;
	debugOffset = 0.8;
	weapon.weaponHitOffset = { -0.05, -0.15, 1 };
	weapon.weaponAngleOffset = { 0.06, 1.3, 0.03 };
	weapon.weaponRadius = 0.25;
	weapon.weaponHeight = 1.5f;

	// 武器のパラメーター初期化
	weapon.position = { 0.07, 0.17, 0.02 };
	weapon.angle = { -1.62, 5.22, 2.89 };

	// アニメーション設定
	player->GetNodePoses(nodePoses);
	player->GetNodePoses(oldNodePoses);
	state = State::Idle;
}

// 更新処理
void Player::Update(float elapsedTime)
{
	// 入力処理
	InputMove(elapsedTime);

	// 速力更新処理
	UpdateVelocity(elapsedTime);

	// 武器のアタッチメント処理
	WeaponAttachment();

	// アニメーション更新処理
	UpdateAnimations(elapsedTime);

	// モデル更新処理
	UpdateTransform();
	player->UpdateTransform(transform);
}

// 描画処理
void Player::Render(RenderContext& rc, ModelRenderer* renderer)
{
	renderer->Draw(ShaderId::Lambert, player); 
	renderer->Draw(ShaderId::Lambert, weapon.model);
}

// GUI描画
void Player::DrawGUI()
{
	ImGui::Begin("Player");

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

	// パラメーター
	if (ImGui::CollapsingHeader("Parameter", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Move Speed:", &moveSpeed, 1.0f, 0, 10); // 移動速度
	}

	ImGui::End();
}

// デバックプリミティブ描画
void Player::RenderDebugPrimitive(ShapeRenderer* renderer)
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
	{
		DirectX::XMFLOAT4X4 weaponTransform;

		DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

		DirectX::XMMATRIX S = DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f);
		DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(
			weapon.angle.x + weapon.weaponAngleOffset.x,
			weapon.angle.y + weapon.weaponAngleOffset.y,
			weapon.angle.z + weapon.weaponAngleOffset.z);
		DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(
			weapon.position.x + weapon.weaponHitOffset.x,
			weapon.position.y + weapon.weaponHitOffset.y,
			weapon.position.z + weapon.weaponHitOffset.z);
		DirectX::XMMATRIX WorldWeapon = S * R * T * weaponWorld;
		DirectX::XMStoreFloat4x4(&weaponTransform, WorldWeapon);

		renderer->DrawCapsule(weaponTransform, weapon.weaponRadius, weapon.weaponHeight, { 1, 0, 0, 1 });
	}
}

//　武器の位置を取得
DirectX::XMFLOAT3 Player::GetWeaponPosition() const
{
	// transformから位置を取得する
	DirectX::XMMATRIX weaponWorld = DirectX::XMLoadFloat4x4(&weapon.transform);

	// オフセットを適用する
	DirectX::XMMATRIX offset = DirectX::XMMatrixTranslation(
		weapon.weaponHitOffset.x,
		weapon.weaponHitOffset.y,
		weapon.weaponHitOffset.z
		);

	DirectX::XMMATRIX finalMatrix = offset * weaponWorld;
	DirectX::XMFLOAT3 position;
	position.x = finalMatrix.r[3].m128_f32[0];
	position.y = finalMatrix.r[3].m128_f32[1];
	position.z = finalMatrix.r[3].m128_f32[2];

	return position;
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
	DirectX::XMFLOAT3 Direction;
	Direction.x = finalMatrix.r[1].m128_f32[0]; // Y軸方向
	Direction.y = finalMatrix.r[1].m128_f32[1];
	Direction.z = finalMatrix.r[1].m128_f32[2];

	return Direction;
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
	DirectX::XMFLOAT3 moveVec = GetMoveVec();

	// 移動処理
	Move(moveVec.x, moveVec.z, moveSpeed);

	// 回転処理
	Turn(elapsedTime, moveVec.x, moveVec.z, turnSpeed);
}

// アニメーション更新処理
void Player::UpdateAnimations(float elapsedTime)
{
	// アニメーション切り替え操作
	DirectX::XMFLOAT3 moveVec = GetMoveVec();
	float moveLength = sqrtf(moveVec.x * moveVec.x + moveVec.y * moveVec.y + moveVec.z * moveVec.z);

	int newAnimationIndex = animationIndex;

	GamePad& gamePad = Input::Instance().GetGamePad();

	switch (state)
	{
	case State::Idle:
		animationLoop = true;
		useRootMotion = false;
		useRootMotionEx = false;
		newAnimationIndex = player->GetAnimationIndex("Idle");

		if (moveLength > 0.0f)
		{
			state = State::Walk;
		}

		if (gamePad.GetButtonDown() & GamePad::BTN_A)
		{
			state = State::Attack;
		}

		break;
	case State::Walk:
		animationLoop = true;
		useRootMotion = false;
		useRootMotionEx = true;
		newAnimationIndex = player->GetAnimationIndex("Walk_F");

		if (moveLength < 0.1f)
		{
			state = State::Idle;
		}

		if (gamePad.GetButtonDown() & GamePad::BTN_A)
		{
			state = State::Attack;
		}

		break;
	case State::Attack:
		animationLoop = false;
		useRootMotion = false;
		useRootMotionEx = true;
		newAnimationIndex = player->GetAnimationIndex("Attack_Right");

		if (moveLength > 0.0f)
		{
			state = State::Walk;
		}

		if (IsFinshedAnimation())
		{
			state = State::Idle;
		}
	}


	// アニメーション切り替え判定
	if (animationIndex != newAnimationIndex)
	{
		// 前のアニメーションの姿勢を保存
		oldNodePoses = nodePoses;

		// 新しいアニメーションに切り替え
		animationIndex = newAnimationIndex;
		animationSeconds = 0.0f;

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

	// アニメーション更新処理
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

		// ルートモーション処理
		if (useRootMotion)
		{
			{
				// ルートモーションノード番号取得
				const int rootMotionNodeIndex = player->GetNodeIndex("mixamorig:Hips");

				// 初回、前回、今回のルートモーションの姿勢を取得
				Model::NodePose beginPose, oldPose, newPose;
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, 0, beginPose);
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, oldAnimationSeconds, oldPose);
				player->ComputeAnimation(animationIndex, rootMotionNodeIndex, animationSeconds, newPose);

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
					// ローカル移動値
					localTranslation.x = newPose.position.x - oldPose.position.x;
					localTranslation.y = newPose.position.y - oldPose.position.y;
					localTranslation.z = newPose.position.z - oldPose.position.z;
				}

				DirectX::XMVECTOR LocalTranslation = DirectX::XMLoadFloat3(&localTranslation);

				// ルートモーションを初回の姿勢にする
				nodePoses[rootMotionNodeIndex].position = beginPose.position;

				// ワールド移動値を算出
				DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&transform);
				DirectX::XMVECTOR WorldTranslation = DirectX::XMVector3TransformNormal(LocalTranslation, WorldTransform);
				DirectX::XMFLOAT3 worldTranslation;
				DirectX::XMStoreFloat3(&worldTranslation, WorldTranslation);


				// 位置を更新
				position.x += worldTranslation.x;
				position.y += worldTranslation.y;
				position.z += worldTranslation.z;
			}
		}

		// 腰骨に対応したルートモーション
		if (useRootMotionEx)
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

		//アニメーション時間更新
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
bool Player::IsFinshedAnimation()
{
	if (animationIndex < 0) return false;

	const Model::Animation& animation = player->GetAnimations().at(animationIndex);
	return animationSeconds >= animation.secondsLength;

	return false;
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