#include "SceneGame.h"

#include "System/Graphic/Graphics.h"

#include "Gameplay/Object/Camera/Camera.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "SceneManager.h"

// シェーダー
#include "System/Renderer/Shader/SkyMap/SkyMap.h"

#include <imgui.h>
#include <fstream>

// 初期化
void SceneGame::Initialize()
{
	ID3D11Device* device = Graphics::Instance().GetDevice();
	float screenWidth = Graphics::Instance().GetScreenWidth();
	float screenHeight = Graphics::Instance().GetScreenHeight();

	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// カメラ設定
	camera.SetPerspectiveFov(
		DirectX::XMConvertToRadians(45),	// 画角
		screenWidth / screenHeight,			// 画面アスペクト比
		0.1f,								// ニアクリップ
		1000.0f								// ファークリップ
	);
	camera.SetLookAt(
		{ 0, 10, 10 },		// 視点
		{ 0, 0, 0 },		// 注視点
		{ 0, 1, 0 }			// 上ベクトル
	);

	// ゲーム用のカメラを初期化
	gameCamera = std::make_unique<GameCamera>();
	gameCamera->SetRange(cameraRange);
	gameCamera->SetAngle({ 0.4, 0.0, 0.0 });

	// デバック用のカメラを初期化
	debugCamera = std::make_unique<DebugCamera>();
	debugCamera->SyncCameraToController(camera);

	// ステージ初期化
	stage = std::make_unique<Stage>();

	// プレイヤー初期化
	Player& player = Player::Instance();
	player.Initialize();

	// エネミー初期化
	enemy = std::make_unique<Enemy>();
	enemy->Initialize();

	// ライト設定
	DirectionalLight light;
	// ここに ImGui で決めた数値をそのまま入れる
	light.direction = { -0.882, -0.471f, 0.021f }; // 例：斜め上から
	light.color = { 1.0f, 1.0f, 1.0f }; // 白色

	DirectX::XMVECTOR vDir = DirectX::XMLoadFloat3(&light.direction);
	DirectX::XMStoreFloat3(&light.direction, DirectX::XMVector3Normalize(vDir));

	lightManager.SetDirectionalLight(light);

	// シャドウマップのパラメータ設定
	shadowAttenuation = 0.125f;
	shadowBias = 0.002f;

	// スカイマップ初期化
	skyMap = std::make_unique<SkyMap>(Graphics::Instance().GetDevice());
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	// --- デバッグ用の更新制御 ---
	bool shouldUpdate = !isPaused || stepNextFrame;
	stepNextFrame = false; // 1フレーム送りのフラグはすぐに折る

	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// プレイヤーを取得
	Player& player = Player::Instance();

	// GUIの操作でゲーム用とデバック用のカメラを切り替え
	if (cameraMode == CameraMode::Game)
	{
		// ゲーム用のカメラ更新
		DirectX::XMFLOAT3 target = player.GetPosition();
		target.y += 1.0f; // 注視点を少し上にあげる
		gameCamera->SetTarget(target);

		// ロックオン対象を毎フレーム渡す
		gameCamera->SetLockOnTarget(enemy.get());

		if (gameCamera->IsLockOn())
		{
			// ロックオン中は敵の位置を教える
			DirectX::XMFLOAT3 enemyPos = enemy->GetPosition();
			player.SetLockOnTargetPosition(&enemyPos);
		}
		else
		{
			player.SetLockOnTargetPosition(nullptr);
		}

		gameCamera->Update(elapsedTime);
	}
	else
	{
		// デバック用のカメラ更新
		debugCamera->Update();
		debugCamera->SyncControllerToCamera(camera);
	}

	if (shouldUpdate)
	{
		// ステージ更新
		stage->Update(elapsedTime);

		// プレイヤー更新
		player.Update(elapsedTime);

		// エネミー更新
		enemy->Update(elapsedTime);

		// プレイヤーと敵の当たり判定
		CollisonPlayervsEnemy();

		// プレイヤーの武器と敵の当たり判定
		CollisionPlayerWeaponVsEnemy();

		// 敵の攻撃とプレイヤーの当たり判定
		CollisionEnemyWeaponVsPlayer();
	}
}

// 描画処理
void SceneGame::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();

	// レンダーステート設定
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// カメラ取得
	Camera& camera = CameraManager::Instance().GetMainCamera();

	// プレイヤーを取得
	Player& player = Player::Instance();

	// シャドウマップ取得
	Graphics& graphics = Graphics::Instance();
	ShadowMap* shadowMap = graphics.GetShadowMap();

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.pbrMetalness = this->pbrMetalness;
	rc.pbrRoughness = this->pbrRoughness;
	rc.lightManager = &lightManager;

	// シャドウマップ準備
	//RenderContextに対してシャドウマップの情報を格納する
	ShadowMapData shadowData;
	shadowData.shadowMap = shadowMap->GetShaderResourceView();
	shadowData.shadowSampler = shadowMap->GetSamplerState();
	shadowData.lightViewProjection = lightViewProjection;
	shadowData.shadowColor = DirectX::XMFLOAT4(0, 0, 0, 0.1);
	shadowData.shadowBias = shadowBias;
	shadowData.shadowAttenuation = shadowAttenuation;
	rc.shadowMapData = &shadowData;

	// シャドウマップ描画
	RenderShadowMap();

	Graphics::Instance().SetRenderTargets();

	// 3Dモデル描画
	{
		// ステージ描画
		stage->Render(rc, modelRenderer);

		// プレイヤー描画
		player.Render(rc, modelRenderer);

		// エネミー描画
		enemy->Render(rc, modelRenderer);

		// 全モデル描画
		modelRenderer->Render(rc);
	}

	// 2Dスプライト描画
	{
		// スカイマップ描画
		Graphics::Instance().SetRenderTargets();
		skyMap->Draw(rc);
	}

	// デバック描画
	{
		bool showPlayerWeapon = player.GetAnimSequence().IsHitActive(
			player.GetCurrentState(),
			player.GetCurrentAnimationSeconds()
		);

		bool showEnemyWeapon = enemy->GetAnimSequence().IsHitActive(
			enemy->GetCurrentState(),
			enemy->GetCurrentAnimationSeconds()
		);

		player.RenderDebugPrimitive(shapeRenderer, showPlayerWeapon);
		enemy->RenderDebugPrimitive(shapeRenderer, showEnemyWeapon);

		// シェイプレンダラ描画
		shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection());
	}
}

// シャドウマップ描画
void SceneGame::RenderShadowMap()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
	PrimitiveRenderer* primitiveRenderer = Graphics::Instance().GetPrimitiveRenderer();
	ShapeRenderer* shapeRenderer = Graphics::Instance().GetShapeRenderer();
	ModelRenderer* modelRenderer = Graphics::Instance().GetModelRenderer();
	Camera& camera = CameraManager::Instance().GetMainCamera();
	Graphics& graphics = Graphics::Instance();
	ShadowMap* shadowMap = graphics.GetShadowMap();

	// プレイヤーを取得
	Player& player = Player::Instance();

	// レンダーステート設定
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
	dc->RSSetState(renderState->GetRasterizerState(RasterizerState::SolidCullNone));

	// 描画コンテキスト設定
	RenderContext rc;
	rc.deviceContext = dc;
	rc.renderState = renderState;
	rc.camera = &camera;
	rc.lightManager = &lightManager;

	//１カメラをライトから見た情報に変更
	{
		// 最初のカメラの情報を保存する
		camera.SaveCamera();

		// ライト方向を取得
		DirectX::XMFLOAT3 lightDir = lightManager.GetDirectionalLight().direction;

		// ライトビュー行列を計算
		// ライト方向の反対側にライトを配置
		DirectX::XMFLOAT3 lightPos = { -lightDir.x * 50.0f, -lightDir.y * 50.0f, -lightDir.z * 50.0f };
		DirectX::XMFLOAT3 lightTarget = { 0, 0, 0 }; // 原点を向くようにする
		DirectX::XMFLOAT3 lightEyeUp = { 0, 1, 0 }; // 上方向を向く

		// ★追加：ライトが真上または真下（Y軸とほぼ平行）を向いているかチェック
		// 内積に近い判定。0.99f 以上の場合はほぼ重なっているとみなす
		if (fabsf(lightDir.y) > 0.999f) {
			// 真上・真下を向いているときは、仮の上方向を「前」にする
			lightEyeUp = { 0, 0, 1 };
		}

		camera.SetLookAt(lightPos, lightTarget, lightEyeUp);

		// ライトプロジェクション行列を計算
		//※調節する時近平面と遠平面の差が大きいとジャギが出やすい。
		//※ジャギを抑えるには解像度を上げる・バイアスを小さくするなど
		camera.SetOrthographic(
			35.0,   // 横幅
			35.0,   // 高さ
			20.0f,    // 近平面
			100.0f // 遠平面
		);

		DirectX::XMFLOAT4X4 view = camera.GetView();
		DirectX::XMFLOAT4X4 projaection = camera.GetProjection();
		DirectX::XMMATRIX V = DirectX::XMLoadFloat4x4(&view);
		DirectX::XMMATRIX P = DirectX::XMLoadFloat4x4(&projaection);

		// ライトビュープロジェクション行列を計算して保存
		// シャドウマップ用の情報に入れる
		DirectX::XMMATRIX lightViewProjection_M = DirectX::XMMatrixMultiply(V, P);
		DirectX::XMStoreFloat4x4(&lightViewProjection, lightViewProjection_M);
	}

	shadowMap->Active(dc);
	{
		// ステージ描画
		stage->Render(rc, modelRenderer);

		// プレイヤー描画
		player.Render(rc, modelRenderer);

		// エネミー描画
		enemy->Render(rc, modelRenderer);

		// 全モデル描画
		modelRenderer->Render(rc);
	}
	shadowMap->Deactive(dc);

	//２変更したカメラの情報を元に戻すこと
	camera.ResetCamera();
}

// GUI描画
void SceneGame::DrawGUI()
{
	// プレイヤーを取得
	Player& player = Player::Instance();

	ImGui::Begin("Game Debug Control");
	{
		// 一時停止ボタン
		if (isPaused) {
			if (ImGui::Button("Resume (F5)")) isPaused = false;
		}
		else {
			if (ImGui::Button("Pause (F5)")) isPaused = true;
		}

		ImGui::SameLine();

		// 1フレーム送りボタン（ポーズ中のみ有効）
		if (!isPaused)
		{
			// ポーズ中じゃない時は、見た目をグレー（無効っぽく）にする
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		}

		if (ImGui::Button("Step Frame (F6)")) {
			// ポーズ中のみ処理を受け付ける
			if (isPaused) {
				stepNextFrame = true;
			}
		}

		if (!isPaused)
		{
			// 色の設定を元に戻す
			ImGui::PopStyleColor(3);
		}

		ImGui::Text("Status: %s", isPaused ? "PAUSED" : "RUNNING");
	}
	ImGui::End();

	ImGui::Begin("Camera System");
	{
		// モード切り替え用のラジオボタン
		int mode = static_cast<int>(cameraMode);
		if (ImGui::RadioButton("Game (3rd Person)", &mode, 0)) {
			cameraMode = CameraMode::Game;
		}
		ImGui::SameLine();

		if (ImGui::RadioButton("Debug (Free)", &mode, 1)) {
			cameraMode = CameraMode::Debug;
			// 切り替えた瞬間に現在のカメラ位置をデバッグ用カメラに同期させる
			debugCamera->SyncCameraToController(CameraManager::Instance().GetMainCamera());
		}
		ImGui::Separator();

		ImGui::DragFloat("Distance", &cameraRange, 0.1f);
		gameCamera->SetRange(cameraRange);
	}
	ImGui::End();

	ImGui::Begin("Shader");
	{
		if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DirectionalLight& light = lightManager.GetDirectionalLight();

			// 1. GUIで値を操作
			if (ImGui::DragFloat3("light direction", &light.direction.x, 0.01f, -1.0f, 1.0f))
			{
				// 2. 値が変更されたら、ベクトルを数学的に正しい状態（長さ1）に直す
				DirectX::XMVECTOR vDir = DirectX::XMLoadFloat3(&light.direction);

				// ベクトルの長さがほぼ0（全要素0など）でないかチェック
				if (DirectX::XMVector3LengthSq(vDir).m128_f32[0] > 0.0001f)
				{
					vDir = DirectX::XMVector3Normalize(vDir);
					DirectX::XMStoreFloat3(&light.direction, vDir);
				}
				else
				{
					// もし完全に0になったらデフォルトの向き（真下など）に戻す安全策
					light.direction = { 0.0f, -1.0f, 0.0f };
				}
			}

			ImGui::ColorEdit3("light color", &light.color.x);

			// 3. 補正した値をマネージャーにセット
			lightManager.SetDirectionalLight(light);
		}

		if (ImGui::CollapsingHeader("PBR", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::DragFloat("Adjust Metalness", &pbrMetalness, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat("Adjust Roughness", &pbrRoughness, 0.01f, -1.0f, 1.0f);
		}

		if (ImGui::CollapsingHeader("ShadowMap", ImGuiTreeNodeFlags_DefaultOpen))
		{
			Graphics& graphics = Graphics::Instance();
			ShadowMap* shadowMap = graphics.GetShadowMap();
			RenderContext rc;

			ImGui::SliderFloat("Shadow Attenuation", &shadowAttenuation, 0.0f, 1.0f);
			ImGui::SliderFloat("Shadow Bias", &shadowBias, 0.0f, 0.01f);

			ImGui::Text("texture");
			ImGui::Image(shadowMap->GetShaderResourceView(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });
		}
	}
	ImGui::End();

	player.DrawGUI();
	enemy->DrawGUI();
}

// プレイヤーと敵の当たり判定
void SceneGame::CollisonPlayervsEnemy()
{
	// プレイヤーを取得
	Player& player = Player::Instance();
	DirectX::XMFLOAT3 outPositionA, outPositionB;
	if (Collision::IntersectCapsuleVsCapsule(
		player.GetPosition(),
		player.GetCapsuleDirection(),
		player.GetHeight(),
		player.GetRadius(),
		player.GetWeight(),
		enemy->GetPosition(),
		enemy->GetCapsuleDirection(),
		enemy->GetHeight(),
		enemy->GetRadius(),
		enemy->GetWeight(),
		outPositionA,
		outPositionB
	))
	{
		// 押し出し処理
		player.SetPosition(outPositionA);
		enemy->SetPosition(outPositionB);
	}
}

// 武器と敵の当たり判定
void SceneGame::CollisionPlayerWeaponVsEnemy()
{
	// プレイヤーを取得
	Player& player = Player::Instance();
	float currentSec = player.GetCurrentAnimationSeconds(); // 既にある
	if (!player.GetAnimSequence().IsHitActive(player.GetCurrentState(), currentSec))
		return;

	DirectX::XMFLOAT3 outPositionA, outPositionB;
	if (Collision::IntersectCapsuleVsCapsule(
		player.GetWeaponPosition(),
		player.GetWeaponDirection(),
		player.GetWeaponHeight(),
		player.GetWeaponRadius(),
		1.0f, // 武器の重さ(適当に設定)
		enemy->GetPosition(),
		enemy->GetCapsuleDirection(),
		enemy->GetHeight(),
		enemy->GetRadius(),
		enemy->GetWeight(),
		outPositionA,
		outPositionB
	))
	{
		// ダメージ処理
		auto state = player.GetCurrentState();
		const auto& config = AnimationStateManager<PlayerAnimationState>::Instance().GetConfig(state);

		if (config->damageRate > 0.0f)
		{
			// CalculateAttackResult に「実数値」を渡す
			AttackResult res = player.CalculateAttackResult(config->damageRate, config->poiseValue);

			// 敵の強靭値を実数値（res.poiseDamage）で減らす
			enemy->SetLastDamage(res.damage);
			enemy->ApplyDamage(res.damage, 0.3f, res.poiseDamage);
		}
	}
}

// 敵の武器とプレイヤーの当たり判定
void SceneGame::CollisionEnemyWeaponVsPlayer()
{
	Player& player = Player::Instance();

	float currentSec = enemy->GetCurrentAnimationSeconds();
	auto state = enemy->GetCurrentState();
	const auto& config = AnimationStateManager<EnemyAnimationState>::Instance().GetConfig(state);


	// 敵のi番目の武器 vs プレイヤーの本体
	DirectX::XMFLOAT3 outPositionA, outPositionB;
	for (int i = 0; i < 2; ++i)
	{
		// インデックスからHandTypeに変換（0＝右手、1＝左手）
		HandType hand = (i == 0) ? HandType::RightHand : HandType::LeftHand;

		// その手のHitBoxがアクティブか確認
		const AnimTrack* activeTrack = enemy->GetAnimSequence().GetActiveHitTrack(state, currentSec, hand);
		if (!activeTrack) continue;

		if (Collision::IntersectCapsuleVsCapsule(
			enemy->GetWeaponPosition(i),    
			enemy->GetWeaponDirection(i),   
			enemy->GetWeaponHeight(i),      
			enemy->GetWeaponRadius(i),      
			1.0f,                           
			player.GetPosition(),
			player.GetCapsuleDirection(),
			player.GetHeight(),
			player.GetRadius(),
			player.GetWeight(),
			outPositionA,
			outPositionB
		))
		{
			// JsonからAttackDataのダメージを持ってくる
			float finalDamage = config->damageRate * activeTrack->damageRate;
			float finalPoiseValue = config->poiseValue * activeTrack->poiseRate;
			if (config->damageRate > 0.0f)
			{
				AttackResult res = enemy->CalculateAttackResult(finalDamage, finalPoiseValue);
				player.SetLastDamage(res.damage);
				player.ApplyDamage(res.damage, config->invincible, res.poiseDamage);
			}
		}

	}

	// 手や体の攻撃
	for (auto& sphereInfo : enemy->GetActiveSphereHits())
	{
		// インデックスからHandTypeに変換（0＝右手、1＝左手）
		HandType hand = HandType::Body;

		// その手のHitBoxがアクティブか確認
		const AnimTrack* activeTrack = enemy->GetAnimSequence().GetActiveHitTrack(state, currentSec, hand);
		if (!activeTrack) continue;

		DirectX::XMFLOAT3 outPos;
		if (Collision::IntersectSphereVsCapsule(
			sphereInfo.position, sphereInfo.radius,
			player.GetPosition(),
			player.GetCapsuleDirection(),
			player.GetHeight(),
			player.GetRadius(),
			outPos))
		{
			// JsonからAttackDataのダメージを持ってくる
			float finalDamage = config->damageRate * activeTrack->damageRate;
			float finalPoiseValue = config->poiseValue * activeTrack->poiseRate;
			if (config->damageRate > 0.0f)
			{
				AttackResult res = enemy->CalculateAttackResult(finalDamage, finalPoiseValue);
				player.SetLastDamage(res.damage);
				player.ApplyDamage(res.damage, config->invincible, res.poiseDamage);
			}
		}
	}
}