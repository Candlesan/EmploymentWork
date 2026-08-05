#include "SceneGame.h"

#include "System/Graphic/Graphics.h"
#include "System/Audio/Audio.h"
#include "System/Effect/EffectManager.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"
#include "System/Core/Input/Input.h"
#include "System/UI/Dialog.h"

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

	// フレームバッファ
	framebuffers[0] = std::make_unique<framebuffer>(device, screenWidth, screenHeight, true);
	bit_block_transfer = std::make_unique<fullscreen_quad>(device);

	// ブルーム関連
	bloomer = std::make_unique<bloom>(device, screenWidth, screenHeight);
	GpuResourceUtils::LoadPixelShader(device, "Data/Shader/final_pass_ps.cso", pixel_shaders[0].ReleaseAndGetAddressOf());
	bloomer->bloom_intensity = bloom_intensity;
	bloomer->bloom_extraction_threshold = bloom_extraction;

	// シャドウマップのパラメータ設定
	shadowAttenuation = 0.125f;
	shadowBias = 0.002f;

	// スカイマップ初期化
	skyMap = std::make_unique<SkyMap>(Graphics::Instance().GetDevice());

	Load("Data/Model/Json/Material/Player/player_material.json", &player);

	// エフェクトエディター初期化
	effectEditor = std::make_unique<EffectEditor>();
	effectEditor->Initialize(device);

	// 自作エフェクトマネージャー初期化
	CustomEffectManager::Instance().Initialize(device);
}

// 終了化
void SceneGame::Finalize()
{
	Player::Instance().Finalize();
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 演出用の倍率
	float gameplayEffectScale = 1.0f;

	// ジャスト回避したときのスロータイマーが動いてるなら
	if (Player::Instance().GetJustAvoidSlowTimer() > 0.0f)
	{
		// スロー中なら、演出用の倍率を落とす
		gameplayEffectScale = 1.0f * 0.3f; // 元のコードの意図を維持
	}

	float delta = elapsedTime * timeScale * gameplayEffectScale;

	// --- キー入力：F5/F6 のエッジ検出（押下の立ち上がり） ---
	{
		// F5: 一時停止トグル
		bool f5Down = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
		if (f5Down && !wasF5DownLastFrame)
		{
			isPaused = !isPaused;
		}
		wasF5DownLastFrame = f5Down;

		// F6: ポーズ中に1フレーム進める
		bool f6Down = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
		if (f6Down && !wasF6DownLastFrame)
		{
			if (isPaused)
			{
				stepNextFrame = true;
			}
		}
		wasF6DownLastFrame = f6Down;
	}
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

		gameCamera->Update(elapsedTime, stage->GetTriangles(), stage->GetAreas());
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

		// 当たり判定更新
		collisonManager.Update(player, *enemy);

		// デバック表示のON/OFFを切り替える関数
		ShowDebugSwitch();

		// エフェクトエディターの更新
		effectEditor->Update(Graphics::Instance().GetDeviceContext(), elapsedTime);

		//// Xキーを押した瞬間に降雪エフェクトを発生させる
		//if (GetAsyncKeyState('P') & 0x8000)
		//{
		//	DirectX::XMFLOAT3 pos = DirectX::XMFLOAT3((rand() % 30 - 15) * 0.1f, (rand() % 30 * 0.1f + 1) + 10.0f, (rand() % 30 - 15) * 0.1f + 3);
		//	int max = 100;
		//	for (int i = 0; i < max; i++)
		//	{
		//		//	発生位置
		//		DirectX::XMFLOAT3 p = { 0,0,0 };
		//		p.x = pos.x + (rand() % 10001 - 5000) * 0.01f;
		//		p.y = pos.y;
		//		p.z = pos.z + (rand() % 10001 - 5000) * 0.01f;
		//		//	発生方向
		//		DirectX::XMFLOAT3 v = { 0,0,0 };
		//		v.y = -(rand() % 10001) * 0.0002f - 0.002f;
		//		//	力
		//		DirectX::XMFLOAT3 f = { 0,0,0 };
		//		f.x = (rand() % 10001) * 0.00001f + 0.1f;
		//		f.z = (rand() % 10001 - 5000) * 0.00001f;
		//		//	大きさ
		//		DirectX::XMFLOAT2 s = { .2f,.2f };


		// エフェクト更新処理 
		EffectManager::Instance().Update(delta);

		// 自作エフェクトマネージャ更新
		CustomEffectManager::Instance().Update(delta);
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

	// デバック描画
	{
		bool showPlayerWeapon = false;

		// アニメーションリストを取得し、空でなければ処理する！
		const auto& animations = player.GetAnimSequence().GetAnimations();
		if (!animations.empty())
		{
			for (const auto& data : animations) {
				if (data.name == player.GetCurrentState()) {
					for (const auto& r : data.ranges) {
						if (r.isHitBox && player.GetAnimSequence().GetRange(r.name)) {
							showPlayerWeapon = true;
							break;
						}
					}
					break;
				}
			}
		}

		player.RenderDebugPrimitive(shapeRenderer, showPlayerWeapon);
		enemy->RenderDebugPrimitive(shapeRenderer, false);

	}

	// 3Dモデル描画
	{
		framebuffers[0]->clear(dc);
		framebuffers[0]->activate(dc);

		// ステージ描画
		stage->Render(rc, modelRenderer);

		// プレイヤー描画
		player.Render(rc, modelRenderer);

		// エネミー描画
		enemy->Render(rc, modelRenderer);

		// 全モデル描画
		modelRenderer->Render(rc);

		// スカイマップ描画
		skyMap->Draw(rc);

		// エフェクト描画 
		EffectManager::Instance().Render(camera.GetView(), camera.GetProjection());

		// エフェクト描画
		effectEditor->Render(rc, modelRenderer);

		// 自作エフェクトマネージャー描画
		CustomEffectManager::Instance().Render(rc);

		// メッシュVFX描画
		effectEditor->RenderMeshVFX(modelRenderer);

		// 自作エフェクトマネージャーのメッシュVFX描画
		CustomEffectManager::Instance().RenderMeshVFX(modelRenderer);

		modelRenderer->Render(rc);

		// シェイプレンダラ描画
		shapeRenderer->DrawSphere({ 5.0f, 1.0f, 0.0f }, 1.0f, { 1.0f, 0.0f, 1.0f, 1.0f });

		if (showDebugPrimitive)
		{
			shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection());
		}

		framebuffers[0]->deactivate(dc);

		// ブルーム描画
		bloomer->make(dc, framebuffers[0]->shader_resource_views[0].Get(), renderState);

		ID3D11ShaderResourceView* shader_resource_views[] =
		{
			framebuffers[0]->shader_resource_views[0].Get(),
			bloomer->shader_resource_view(),
		};

		Graphics::Instance().SetRenderTargets();

		dc->OMSetBlendState(renderState->GetBlendState(BlendState::Opaque), nullptr, 0xFFFFFFFF);
		dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::TestAndWrite), 0);

		bloomer->set_constant_buffer(dc);
		bit_block_transfer->blit(dc, shader_resource_views, 0, 2, pixel_shaders[0].Get());

		// プリミティブ描画
		primitiveRenderer->Render(dc, camera.GetView(), camera.GetProjection(),
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	}

	// 2Dスプライト描画
	{
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

	// デバック用の便利機能のON/Off
	if (showDebugOption)
	{
		ImGui::Begin(u8"デバック");
		{
			// 一時停止ボタン
			if (isPaused) {
				if (ImGui::Button(u8"再開 (F5)")) isPaused = false;
			}
			else {
				if (ImGui::Button(u8"一時停止 (F5)")) isPaused = true;
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

			if (ImGui::Button(u8"1フレーム進める (F6)")) {
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
			ImGui::Text(u8"再生速度");

			// 0.01 (ほぼ停止) から 1.0 (通常) までのスライダー
			ImGui::SliderFloat(u8"再生スケール", &timeScale, 0.01f, 1.0f, "%.2f");

			if (ImGui::Button(u8"再生時間を元に戻す")) {
				timeScale = 1.0f;
			}
		}
		ImGui::End();

		ImGui::Begin(u8"カメラ");
		{
			if (ImGui::Checkbox(u8"Y軸反転", &invertY))
			{
				gameCamera->SetInvertY(invertY);
			}
			ImGui::Separator();

			// モード切り替え用のラジオボタン
			int mode = static_cast<int>(cameraMode);
			if (ImGui::RadioButton(u8"ゲームカメラ (サードパーソン)", &mode, 0)) {
				cameraMode = CameraMode::Game;
			}
			ImGui::SameLine();

			if (ImGui::RadioButton(u8"デバックカメラ (フリーカメラ)", &mode, 1)) {
				cameraMode = CameraMode::Debug;
				// 切り替えた瞬間に現在のカメラ位置をデバッグ用カメラに同期させる
				debugCamera->SyncCameraToController(CameraManager::Instance().GetMainCamera());
			}
			ImGui::Separator();

			ImGui::DragFloat(u8"距離", &cameraRange, 0.1f);
			gameCamera->SetRange(cameraRange);
		}
		ImGui::End();

		ImGui::Begin(u8"シェーダー");
		{
			if (ImGui::CollapsingHeader(u8"ライト", ImGuiTreeNodeFlags_DefaultOpen))
			{
				DirectionalLight& light = lightManager.GetDirectionalLight();

				// 1. GUIで値を操作
				if (ImGui::DragFloat3(u8"ライトの向き", &light.direction.x, 0.01f, -1.0f, 1.0f))
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

				ImGui::ColorEdit3(u8"ライトの色", &light.color.x);

				// 3. 補正した値をマネージャーにセット
				lightManager.SetDirectionalLight(light);
			}

			if (ImGui::CollapsingHeader(u8"PBR（物理ベース）", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat(u8"金属度", &pbrMetalness, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat(u8"粗さ", &pbrRoughness, 0.01f, -1.0f, 1.0f);
			}

			if (ImGui::CollapsingHeader(u8"シャドウマップ", ImGuiTreeNodeFlags_DefaultOpen))
			{
				Graphics& graphics = Graphics::Instance();
				ShadowMap* shadowMap = graphics.GetShadowMap();
				RenderContext rc;

				ImGui::SliderFloat(u8"影の減衰", &shadowAttenuation, 0.0f, 1.0f);
				ImGui::SliderFloat(u8"シャドウバイアス", &shadowBias, 0.0f, 0.01f);

				ImGui::Text(u8"テクスチャ");
				ImGui::Image(shadowMap->GetShaderResourceView(), { 256, 256 }, { 0, 0 }, { 1, 1 }, { 1, 1, 1, 1 });
			}

			if (ImGui::CollapsingHeader(u8"ブルーム", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat(u8"ブローム抽出閾値", &bloom_extraction, 0.01f, +0.0f, +5.0f);
				ImGui::DragFloat(u8"ブローム強度", &bloom_intensity, 0.01f, +0.0f, +5.0f);
				ImGui::DragFloat(u8"露出", &bloomer->exposure, 0.01f, 0.0f, 1.0f);

				bloomer->bloom_extraction_threshold = bloom_extraction;
				bloomer->bloom_intensity = bloom_intensity;

				ImGui::Separator();

				// グロー抽出
				ImGui::Text(u8"グロー抽出");
				ImGui::Image((ImTextureID)bloomer->get_glkow_extraction(), ImVec2(256, 256));

				// ガウシアンブラー各段階
				for (size_t i = 0; i < bloom::downsampled_count; ++i)
				{
					ImGui::Text(u8"ガウシアンブラー [%zu][0]", i);
					ImGui::Image((ImTextureID)bloomer->get_gaussian_blur(i, 0), ImVec2(256, 256));
					ImGui::Text(u8"ガウシアンブラー [%zu][1]", i);
					ImGui::Image((ImTextureID)bloomer->get_gaussian_blur(i, 1), ImVec2(256, 256));
				}
			}

			ImGui::Separator();
			ImGui::Text(u8"---- マテリアル リアルタイム編集 ----");

			// 日本語ラベルでモデルごとのマテリアルエディタを呼び出す
			DrawModelMaterialEditor(u8"プレイヤー：服・髪のマテリアル", &player);
			DrawModelMaterialEditor(u8"敵：体・鎧のマテリアル", enemy.get());
		}
		ImGui::End();

		ImGui::Begin(u8"当たり判定");
		{
			// SceneGame.cpp の DrawGUI() の中
			ImGui::Checkbox(u8"ジャスト回避テスト用の球を配置", &showDebugSphere);
		}
		ImGui::End();
	}

	// エフェクトエディターの表示
	if (showEffectEditor)
	{
		effectEditor->DrawGUI();

		ImGui::Begin(u8"エフェクトマネージャーテスト");
		{
			if (ImGui::Button(u8"エフェクトJSONをロード"))
			{
				char filepath[MAX_PATH] = { 0 };
				HWND hwnd = Graphics::Instance().GetWindowHandle();

				// ダイアログを開いてJSONファイルを選択[cite: 25]
				if (Dialog::OpenFileName(filepath, MAX_PATH, "JSON File\0*.json\0\0", "エフェクト読込", hwnd) == DialogResult::OK)
				{
					// パスからファイル名を取り出してエフェクト名にする
					char filename[MAX_PATH];
					_splitpath_s(filepath, nullptr, 0, nullptr, 0, filename, MAX_PATH, nullptr, 0);

					// マネージャーにロード＆キャッシュさせる！[cite: 22, 23]
					CustomEffectManager::Instance().LoadEffectConfig(filename, filepath);
				}
			}

			ImGui::Separator();

			// プレイヤーの足元にテスト再生してみるボタン
			char effectNameBuf[128];
			strncpy_s(effectNameBuf, effectName.c_str(), sizeof(effectNameBuf));
			if (ImGui::InputText(u8"エフェクト名入力", effectNameBuf, sizeof(effectNameBuf)))
			{
				effectName = effectNameBuf;
			}
			ImGui::Separator();

			// キャッシュしておいたエフェクトを再生
			if (ImGui::Button(u8"エフェクト再生"))
			{
				CustomEffectManager::Instance().PlayEffect(effectName.c_str(), { 0.0f, 0.0f, 0.0f });
			}
		}
		ImGui::End();
	}

	player.DrawGUI();
	enemy->DrawGUI();
}

void SceneGame::DrawModelMaterialEditor(const char* label, Character* character)
{
	if (!character) return;

	// 親クラスのポインタ経由で、相手が誰であれモデルを取得する
	auto model = character->GetModel();
	if (!model) return; // モデルを持っていないキャラなら何もしない

	// 共通のマテリアル編集スライダー描画ロジック
	if (ImGui::CollapsingHeader(label))
	{
		auto& materials = model->GetMaterials();

		// 保存・読み込みボタン
		if (ImGui::Button(u8"保存"))
		{
			char path[MAX_PATH] = "";
			if (Dialog::SaveFileName(
				path,
				MAX_PATH,
				"JSON Files\0*.json\0\0",
				u8"マテリアルを保存",
				"json") == DialogResult::OK)
			{
				Save(path, character);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(u8"読み込み"))
		{
			char path[MAX_PATH] = "";
			if (Dialog::OpenFileName(
				path,
				MAX_PATH,
				"JSON Files\0*.json\0\0",
				u8"マテリアルを読み込む") == DialogResult::OK)
			{
				Load(path, character);
			}
		}

		// 各マテリアルごとの編集スライダー
		for (size_t i = 0; i < materials.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::Text(u8"マテリアル: %s", materials[i].name.c_str());
			ImGui::SliderFloat(u8"粗さ", &materials[i].adjustRoughness, -1.0f, 1.0f);
			ImGui::SliderFloat(u8"金属度", &materials[i].adjustMetalness, -1.0f, 1.0f);
			ImGui::PopID();
		}
	}
}

void SceneGame::Save(const std::string& path, Character* character)
{
	auto model = character->GetModel();
	if (!model) return;

	nlohmann::json j;
	auto& materials = model->GetMaterials();

	for (auto& mat : materials)
	{
		nlohmann::json entry;
		entry["name"] = mat.name;
		entry["roughness"] = mat.adjustRoughness;
		entry["metalness"] = mat.adjustMetalness;
		j["materials"].push_back(entry);
	}

	std::ofstream file(path);
	file << j.dump(4);
}
void SceneGame::Load(const std::string& path, Character* character)
{
	std::ifstream file(path);
	if (!file.is_open()) return;

	nlohmann::json j = nlohmann::json::parse(file);

	auto model = character->GetModel();
	auto& materials = model->GetMaterials();

	for (auto& entry : j["materials"])
	{
		std::string name = entry["name"];
		// マテリアル名でマッチングして値を上書き
		for (auto& mat : materials)
		{
			if (mat.name == name)
			{
				mat.adjustRoughness = entry["roughness"];
				mat.adjustMetalness = entry["metalness"];
				break;
			}
		}
	}
}

// 画面比率修正関数
void SceneGame::OnResize(float width, float height)
{
	// カメラの比率を直す
	Camera& camera = CameraManager::Instance().GetMainCamera();
	camera.SetPerspectiveFov(DirectX::XMConvertToRadians(45), width / height, 0.1f, 1000.0f);

	ID3D11Device* device = Graphics::Instance().GetDevice();

	framebuffers[0] = std::make_unique<framebuffer>(device, width, height, true);

	// ブルームを新しいサイズで丸ごと生成し直す
	bloomer = std::make_unique<bloom>(device, width, height);

	// 初期化時に設定していたパラメータを再設定
	bloomer->bloom_intensity = bloom_intensity;
	bloomer->bloom_extraction_threshold = bloom_extraction;
}

// デバック表示のON/OFFを切り替える関数
void SceneGame::ShowDebugSwitch()
{
	// F2でデバックの便利機能の表示をON/OFF
	static bool wasF2DownLastFrame = false;
	bool f2Down = (GetAsyncKeyState(VK_F2) & 0x8000) != 0;
	if (f2Down && !wasF2DownLastFrame) {
		showDebugOption = !showDebugOption;
	}
	wasF2DownLastFrame = f2Down;

	// F7でデバッグプリミティブの表示のON/OFF
	static bool wasF7DownLastFrame = false;
	bool f7Down = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
	if (f7Down && !wasF7DownLastFrame) {
		showDebugPrimitive = !showDebugPrimitive;
	}
	wasF7DownLastFrame = f7Down;

	// F7でデバッグプリミティブの表示のON/OFF
	static bool wasF10DownLastFrame = false;
	bool f10Down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
	if (f10Down && !wasF10DownLastFrame) {
		showEffectEditor = !showEffectEditor;
	}
	wasF10DownLastFrame = f10Down;
}
