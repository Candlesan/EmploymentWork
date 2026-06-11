#include "SceneGame.h"

#include "System/Graphic/Graphics.h"
#include "System/Audio/Audio.h"
#include "System/Effect/EffectManager.h"

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

	player.SetEnemy(enemy.get());

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

	// パーティクル生成
	particleManager = std::make_unique<Compute_Particle>(device);
}

// 終了化
void SceneGame::Finalize()
{
	Player::Instance().Finalize();
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	float delta = elapsedTime * timeScale;
	
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
		stage->Update(delta);

		// プレイヤー更新
		player.Update(delta);

		// エネミー更新
		enemy->Update(delta);

		// プレイヤーと敵の当たり判定
		CollisonPlayervsEnemy();

		// プレイヤーの武器と敵の当たり判定
		CollisionPlayerWeaponVsEnemy();

		// 敵の攻撃とプレイヤーの当たり判定
		CollisionEnemyWeaponVsPlayer();

		// 魔法と敵の当たり判定
		CollisionMagicVsEnemy();

		// Xキーを押した瞬間に降雪エフェクトを発生させる
		if (GetAsyncKeyState('P') & 0x8000)
		{
			DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3((rand() % 30 - 15) * 0.1f, (rand() % 30 * 0.1f + 1) + 10.0f, (rand() % 30 - 15) * 0.1f + 3);
			int max = 100;
			for (int i = 0; i < max; i++)
			{
				//	発生位置
				DirectX::XMFLOAT3 p = { 0,0,0 };
				p.x = pos.x + (rand() % 10001 - 5000) * 0.01f;
				p.y = pos.y;
				p.z = pos.z + (rand() % 10001 - 5000) * 0.01f;
				//	発生方向
				DirectX::XMFLOAT3 v = { 0,0,0 };
				v.y = -(rand() % 10001) * 0.0002f - 0.002f;
				//	力
				DirectX::XMFLOAT3 f = { 0,0,0 };
				f.x = (rand() % 10001) * 0.00001f + 0.1f;
				f.z = (rand() % 10001 - 5000) * 0.00001f;
				//	大きさ
				DirectX::XMFLOAT2 s = { .2f,.2f };

				// 発生させるパーティクルのパラメータを設定
				compute_particle_system::emit_particle_data data;

				//	更新タイプ
				data.parameter.x = 12;
				data.parameter.y = 5.0f;
				//	発生位置
				data.position.x = p.x;
				data.position.y = p.y;
				data.position.z = p.z;
				//	発生方向
				data.velocity.x = v.x;
				data.velocity.y = v.y;
				data.velocity.z = v.z;
				//	加速力
				data.acceleration.x = f.x;
				data.acceleration.y = f.y;
				data.acceleration.z = f.z;
				//	大きさ
				data.scale.x = s.x;
				data.scale.y = s.y;
				data.scale.z = 0.0f;

				if (particleManager)
				{
					particleManager->Emit(data);
				}
			}
		}

		// パーティクルシステム自体の更新（これは毎フレーム必ず呼ぶ）
		if (particleManager)
		{
			// デバイスコンテキストを取得して渡す（プロジェクトの仕様に合わせてな）
			ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
			particleManager->Update(dc, elapsedTime);
		}

		// エフェクト更新処理 
		EffectManager::Instance().Update(elapsedTime);
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

	// スカイマップ描画
	Graphics::Instance().SetRenderTargets();
	skyMap->Draw(rc);


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

		// エフェクト描画 
		EffectManager::Instance().Render(camera.GetView(), camera.GetProjection());

		//player.RenderTrail(rc);

		// プリミティブ描画
		primitiveRenderer->Render(dc, camera.GetView(), camera.GetProjection(),
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	}

	// 2Dスプライト描画
	{
		particleManager->Render(rc);
	}

	// デバック描画
	{
		bool showPlayerWeapon = false;
		for (auto& data : player.GetAnimSequence().GetAnimations()) {
			if (data.name == player.GetCurrentState()) {
				for (auto& r : data.ranges) {
					// プレイヤーの右手武器の判定が今フレームで有効か？
					if (r.hand == HandType::RightHand && player.GetAnimSequence().GetRange(r.name)) {
						showPlayerWeapon = true;
						break;
					}
				}
				break;
			}
		}

		bool showEnemyWeapon = false;
		for (auto& data : enemy->GetAnimSequence().GetAnimations()) {
			if (data.name == enemy->GetCurrentState()) {
				for (auto& r : data.ranges) {
					// 敵の武器（右手・左手）の判定が今フレームで有効か？
					if ((r.hand == HandType::RightHand || r.hand == HandType::LeftHand)
						&& enemy->GetAnimSequence().GetRange(r.name)) {
						showEnemyWeapon = true;
						break;
					}
				}
				break;
			}
		}

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

		ImGui::Separator();
		ImGui::Text("Time Control");

		// 0.01 (ほぼ停止) から 1.0 (通常) までのスライダー
		ImGui::SliderFloat("Time Scale", &timeScale, 0.01f, 1.0f, "%.2f");

		if (ImGui::Button("Reset Speed")) {
			timeScale = 1.0f;
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

	bool isHit = false;
	const AnimationSequencer::Range* activeRange = nullptr;

	for(auto& data : player.GetAnimSequence().GetAnimations())
	{
		if (data.name == player.GetCurrentState())
		{
			for (auto& r : data.ranges)
			{
				if (r.hand == HandType::RightHand && player.GetAnimSequence().GetRange(r.name))
				{
					isHit = true;
					activeRange = &r;
					break;
				}
			}
		}
	}

	if (!isHit || !activeRange) return;

	for (int i = 0; i < 2; ++i)
	{
		DirectX::XMFLOAT3 outPositionA, outPositionB;
		if (Collision::IntersectCapsuleVsCapsule(
			player.GetWeaponPosition(i),
			player.GetWeaponDirection(i),
			player.GetWeaponHeight(i),
			player.GetWeaponRadius(i),
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
			const AnimationConfig* config = player.GetAnimationConfig(state);

			float finalDamage = activeRange->damageRate;
			float finalPoiseValue = activeRange->poiseRate;

			if (activeRange->damageRate > 0.0f)
			{
				// CalculateAttackResult に「実数値」を渡す
				AttackResult res = player.CalculateAttackResult(activeRange->damageRate, activeRange->poiseRate);

				// 敵の強靭値を実数値（res.poiseDamage）で減らす
				enemy->SetLastDamage(res.damage);
				enemy->ApplyDamage(res.damage, activeRange->invincible, res.poiseDamage);
			}
		}
	}
}

// 敵の武器とプレイヤーの当たり判定
void SceneGame::CollisionEnemyWeaponVsPlayer()
{
	Player& player = Player::Instance();

	float currentSec = enemy->GetCurrentAnimationSeconds();
	auto state = enemy->GetCurrentState();

	const AnimationConfig* config = enemy->GetAnimationConfig(state);

	// 敵のi番目の武器 vs プレイヤーの本体
	DirectX::XMFLOAT3 outPositionA, outPositionB;
	for (int i = 0; i < 2; ++i)
	{
		// インデックスからHandTypeに変換（0＝右手、1＝左手）
		HandType hand = (i == 0) ? HandType::RightHand : HandType::LeftHand;

		// その手のHitBoxがアクティブか確認
		bool isHit = false;
		const AnimationSequencer::Range* activeRange = nullptr;

		for (auto& data : player.GetAnimSequence().GetAnimations())
		{
			if (data.name == player.GetCurrentState())
			{
				for (auto& r : data.ranges)
				{
					if (r.hand == HandType::RightHand && player.GetAnimSequence().GetRange(r.name))
					{
						isHit = true;
						activeRange = &r;
						break;
					}
				}
			}
		}

		if (!isHit || !activeRange) return;

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
			player.SetIsHit(true);
			// JsonからAttackDataのダメージを持ってくる
			float finalDamage = activeRange->damageRate;
			float finalPoiseValue = activeRange->poiseRate;
			bool IsGuarding = player.GetIsGuarding();
			if (IsGuarding) player.SetIsHit(true);
			if (activeRange->damageRate > 0.0f && !IsGuarding)
			{
				AttackResult res = enemy->CalculateAttackResult(finalDamage, finalPoiseValue);
				player.SetLastDamage(res.damage);
				player.ApplyDamage(res.damage, activeRange->invincible, res.poiseDamage);
			}
		}
	}

	// 手や体の攻撃
	for (auto& sphereInfo : enemy->GetActiveSphereHits())
	{
		// インデックスからHandTypeに変換（0＝右手、1＝左手）
		HandType hand = HandType::Body;

		// その手のHitBoxがアクティブか確認
		bool isHit = false;
		const AnimationSequencer::Range* activeRange = nullptr;

		for (auto& data : player.GetAnimSequence().GetAnimations()) {
			if (data.name == player.GetCurrentState()) {
				for (auto& r : data.ranges) {
					// 右手攻撃 かつ 今フレームで有効(GetRange)か？
					if (r.hand == HandType::RightHand && player.GetAnimSequence().GetRange(r.name)) {
						isHit = true;
						activeRange = &r;
						break;
					}
				}
				break;
			}
		}

		if (!isHit || !activeRange) return;

		DirectX::XMFLOAT3 outPos;
		if (Collision::IntersectSphereVsCapsule(
			sphereInfo.position, sphereInfo.radius,
			player.GetPosition(),
			player.GetCapsuleDirection(),
			player.GetHeight(),
			player.GetRadius(),
			outPos))
		{
			player.SetIsHit(true);

			// JsonからAttackDataのダメージを持ってくる
			float finalDamage = activeRange->damageRate;
			float finalPoiseValue = activeRange->poiseRate;
			bool IsGuarding = player.GetIsGuarding();
			bool IsAvoid = player.GetIsAvoid();

			if(IsGuarding) player.SetIsHit(true);
			if (activeRange->damageRate > 0.0f && !IsGuarding && !IsAvoid)
			{
				AttackResult res = enemy->CalculateAttackResult(finalDamage, finalPoiseValue);
				player.SetLastDamage(res.damage);
				player.ApplyDamage(res.damage, activeRange->invincible, res.poiseDamage);
			}
		}
	}
}

// 敵と魔法の当たり判定
void SceneGame::CollisionMagicVsEnemy()
{
	// プレイヤーを取得
	Player& player = Player::Instance();
	MagicManager& magicManager = player.GetMagicManager();

	// 存在する魔法の数だけループ
	for (int i = 0; i < magicManager.GetMagicCount(); ++i)
	{
		MagicBase* magic = magicManager.getMagic(i);

		DirectX::XMFLOAT3 outPos;

		// 魔法と敵の当たり判定
		if (Collision::IntersectSphereVsCapsule(
			magic->GetPosition(), magic->GetRadius(),
			enemy->GetPosition(), enemy->GetCapsuleDirection(), enemy->GetHeight(), enemy->GetRadius(),
			outPos))
		{
			// ダメージを与える
			enemy->ApplyDamage(magic->GetDamage(), 0.3, 10.0f);

			// 魔法自体を消滅させる
			magic->OnTerminate();
		}
	}
}
