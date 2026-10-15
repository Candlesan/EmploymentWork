#include "SceneGame.h"

#include "System/Graphic/Graphics.h"
#include "System/Audio/Audio.h"
#include "System/Effect/EffectManager.h"
#include "System/UI/EffectEditor/EffectManager/CustomEffectManager.h"
#include "System/Core/Input/Input.h"
#include "System/UI/Dialog/Dialog.h"

#include "Gameplay/Object/Camera/Camera.h"
#include "GamePlay/Object/Collision/Collision.h"
#include "GamePlay/Object/Character/Player/Player.h"
#include "Gameplay/Scene/Manager/SceneManager.h"

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
	player.GetLockOnSystem().Initialize(&player, enemy.get());

	player.SetStage(stage.get());
	enemy->SetStage(stage.get());

	// ライト設定
	DirectionalLight light;
	// ここに ImGui で決めた数値をそのまま入れる
	light.direction = CalcLightDirection(); // 例：斜め上から
	light.color = { 1.0f, 1.0f, 1.0f }; // 白色
	light.intensity = 2.0f;

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
	shadowAttenuation = 0;
	shadowBias = 0.001f;

	// スカイマップ初期化
	skyMap = std::make_unique<SkyMap>(Graphics::Instance().GetDevice());

	Load("Data/Model/Json/Material/Player/player_material.json", &player);

	// エフェクトエディター初期化
	effectEditor = std::make_unique<EffectEditor>();
	effectEditor->Initialize(device);

	// 自作エフェクトマネージャー初期化
	CustomEffectManager::Instance().Initialize(device);

	// UIエディター初期化
	uiEditor.Initialize(device);
	uiCanvas.Initialize(device);
	uiCanvas.Load("Data/Json/UI/SceneGame.json");
	playerUI.Initialize(uiCanvas);
	enemyUI.Initialize(uiCanvas);
}

// 終了化
void SceneGame::Finalize()
{
	Player& player = Player::Instance();
	player.SetStage(stage.get());
	enemy->SetStage(stage.get());
	player.Finalize();
}

// 更新処理
void SceneGame::Update(float elapsedTime)
{
	GamePad& gamePad = Input::Instance().GetGamePad();

	// 演出用の倍率
	float gameplayEffectScale = 1.0f;

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
		collisonManager.Update(player, *enemy, *stage);

		// デバック表示のON/OFFを切り替える関数
		ShowDebugSwitch();

		// エフェクトエディターの更新
		effectEditor->Update(Graphics::Instance().GetDeviceContext(), elapsedTime);

		// エフェクト更新処理 
		EffectManager::Instance().Update(delta);

		// 自作エフェクトマネージャ更新
		CustomEffectManager::Instance().Update(delta);

		playerUI.Update(Player::Instance().GetStatus());
		playerUI.UpdateCrosshair(elapsedTime, player.GetLockOnSystem(), camera);

		enemyUI.Update(enemy.get()->GetStatus());

		uiEditor.Update(elapsedTime);
		uiCanvas.Update(elapsedTime);
	}
}

// 描画処理
void SceneGame::Render()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
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
	rc.pbrMetalness = pbrMetalness;
	rc.pbrRoughness = pbrRoughness;
	rc.lightManager = &lightManager;
	rc.ambientIntensity = ambientIntensity;

	// シャドウマップ描画
	RenderShadowMap();

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

#ifdef _DEBUG
		player.RenderDebugPrimitive(shapeRenderer);
		enemy->RenderDebugPrimitive(shapeRenderer);
		stage->RenderDebugPrimitive(shapeRenderer);
#endif // DEBUG

	}

	// 3Dモデル描画
	{
		framebuffers[0]->clear(dc);
		framebuffers[0]->activate(dc);

		// ===== モデル =====
		stage->Render(rc, modelRenderer); // ステージ描画
		player.Render(rc, modelRenderer); // プレイヤー描画
		enemy->Render(rc, modelRenderer); // エネミー描画		

		// 全モデル描画
		modelRenderer->Render(rc); 


		// ===== シェーダー =====
		
		skyMap->Draw(rc); // スカイマップ描画

		// エフェクト描画 (Effekseer)
		EffectManager::Instance().Render(camera.GetView(), camera.GetProjection());

		// エフェクト描画 (自作エフェクトエディター)
		effectEditor->Render(rc, modelRenderer);
		effectEditor->RenderMeshVFX(modelRenderer);

		// エフェクト描画 (ゲーム実行時)
		CustomEffectManager::Instance().Render(rc);
		CustomEffectManager::Instance().RenderMeshVFX(modelRenderer);

		// シェイプ描画
		if (showDebugPrimitive)
		{
			shapeRenderer->Render(dc, camera.GetView(), camera.GetProjection());
		}

		framebuffers[0]->deactivate(dc);

		// ブルーム描画
		bloomer->make(dc, framebuffers[0]->shader_resource_views[0].Get(), renderState);

		// ブルーム設定
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
	}

	// 2Dスプライト描画
	{
		uiEditor.Render(dc);
		uiCanvas.Render(dc);
	}
}

// シャドウマップ描画
void SceneGame::RenderShadowMap()
{
	ID3D11DeviceContext* dc = Graphics::Instance().GetDeviceContext();
	RenderState* renderState = Graphics::Instance().GetRenderState();
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

		// ステージの大きさに収める
		DirectX::XMFLOAT3 vmin = stage->GetVolumeMin();
		DirectX::XMFLOAT3 vmax = stage->GetVolumeMax();

		// ステージの大きさを計算する
		DirectX::XMVECTOR Min = DirectX::XMLoadFloat3(&vmin);
		DirectX::XMVECTOR Max = DirectX::XMLoadFloat3(&vmax);
		DirectX::XMVECTOR Center = DirectX::XMVectorScale(DirectX::XMVectorAdd(Min, Max), 0.5f);
		float radius = DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(Max, Center)));

		DirectX::XMVECTOR L = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&lightDir));
		DirectX::XMFLOAT3 lightPos, lightTarget;
		DirectX::XMStoreFloat3(&lightPos, DirectX::XMVectorSubtract(Center, DirectX::XMVectorScale(L, radius)));
		DirectX::XMStoreFloat3(&lightTarget, Center);

		DirectX::XMFLOAT3 lightEyeUp = { 0, 1, 0 }; // 上方向を向く

		// 計算した内容をセットする
		camera.SetLookAt(lightPos, lightTarget, lightEyeUp);
		camera.SetOrthographic(
			radius * 1.2f,   // 横幅：アリーナの直径
			radius * 1.2f,   // 高さ
			0.0f,            // 近平面：ライト位置から
			radius * 1.2f    // 遠平面：アリーナの反対側まで
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
	Graphics& graphics = Graphics::Instance();
	ShadowMap* shadowMap = graphics.GetShadowMap();
	RenderContext rc;

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

				// 方位と高さは別々に動く（お互いに影響しない）
				ImGui::SliderFloat(u8"ライトの方位", &lightYaw, 0.0f, 360.0f);
				ImGui::SliderFloat(u8"ライトの高さ", &lightPitch, 5.0f, 89.0f);
				ImGui::DragFloat(u8"ライトの強さ", &light.intensity, 0.1f, 0.0f, 10.0f);
				light.direction = CalcLightDirection();

				// 確認用：実際の向き
				ImGui::Text(u8"向き: %.3f, %.3f, %.3f", light.direction.x, light.direction.y, light.direction.z);

				ImGui::ColorEdit3(u8"ライトの色", &light.color.x);
			}

			if (ImGui::CollapsingHeader(u8"PBR（物理ベース）", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat(u8"金属度", &pbrMetalness, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat(u8"粗さ", &pbrRoughness, 0.01f, -1.0f, 1.0f);
				ImGui::DragFloat(u8"環境光の強さ", &ambientIntensity, 0.1f, 0.0f, 10.0f);
			}

			if (ImGui::CollapsingHeader(u8"シャドウマップ", ImGuiTreeNodeFlags_DefaultOpen))
			{

				ImGui::DragFloat(u8"影の減衰", &shadowAttenuation, 0.001f, 0.0f, 1.0f);
				ImGui::DragFloat(u8"シャドウバイアス", &shadowBias, 0.001f, 0.0f, 1.0f);

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
	}

	// エフェクトエディターの表示
	if (showEffectEditor)
	{
		effectEditor->DrawGUI();
	}
	if (showUIEdlitor)
	{
		uiEditor.DrawGUI();
	}

	player.DrawGUI();
	enemy->DrawGUI();
	stage->DrawGUI();
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
				"マテリアルを保存",
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
				"マテリアルを読み込む") == DialogResult::OK)
			{
				Load(path, character);
			}
		}

		// 各マテリアルごとの編集スライダー
		for (size_t i = 0; i < materials.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::Text(u8"マテリアル: %s", materials[i].name.c_str());
			ImGui::DragFloat(u8"粗さ", &materials[i].adjustRoughness, 0.1f, -1.0f, 1.0f);
			ImGui::DragFloat(u8"金属度", &materials[i].adjustMetalness, 0.1f, -1.0f, 1.0f);
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

	// F10でエフェクトエディターとUIエディターの表示のON/OFF
	static bool wasF10DownLastFrame = false;
	bool f10Down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
	if (f10Down && !wasF10DownLastFrame) {
		showEffectEditor = !showEffectEditor;
		showUIEdlitor = !showUIEdlitor;
	}
	wasF10DownLastFrame = f10Down;
}

DirectX::XMFLOAT3 SceneGame::CalcLightDirection() const
{
	float Yaw = DirectX::XMConvertToRadians(lightYaw);
	float Pitch = DirectX::XMConvertToRadians(lightPitch);
	return {
		cosf(Pitch) * sinf(Yaw),
		-sinf(Pitch),
		cosf(Pitch) * cosf(Yaw)
	};
}
