#include "Stage.h"

#include "System/Graphic/Graphics.h"
#include "System/Renderer/ShapeRenderer/ShapeRenderer.h"
#include "System/UI/Dialog/Dialog.h"
#include "json.hpp"


#include <fstream>
#include <imgui.h>
#include <filesystem>


// コンストラクタ
Stage::Stage()
{
	// デバイス取得
	ID3D11Device* device = Graphics::Instance().GetDevice();

	// ステージモデル読み込み
	stage = std::make_shared<Model>(device, "Data/Model/Stage/Stage/circle_of_death.gltf");
	scale.x = scale.y = scale.z = 0.025f;

	// 装飾物を読み込む
	LoadProps(PropsFilePath);

	// 当たり判定を作る
	BuildCollision();

	CustomEffectManager::Instance().LoadEffectConfig("Lamp", "Data/Json/Effect/LampLight.json");
}

Stage::~Stage()
{
	// エフェクトを停止する
	CustomEffectManager::Instance().StopAll();
	for (auto& prop : props)
	{
		prop.LampLight = InvalidEffect;
	}
}

// 更新処理
void Stage::Update(float elapsedTime)
{
	transform = MakeTransform(position, angle, scale); // 本体
	stage->UpdateTransform(transform); // モデルの行列を更新する

	time += elapsedTime;


	// 各装飾の行列を更新する
	for (auto& prop : props)
	{
		DirectX::XMFLOAT3 pos = prop.position;
		DirectX::XMFLOAT3 angle = prop.angle;

		pos.y += sinf(time * prop.bobSpeed * DirectX::XM_2PI) * prop.bobHeight;
		angle.y += DirectX::XMConvertToRadians(prop.rotateSpeed) * time;

		prop.transform = MakeTransform(pos, angle, prop.scale);
		prop.model->UpdateTransform(prop.transform);

		if (prop.name == u8"ランプ" && prop.LampLight == InvalidEffect)
		{
			prop.LampLight = CustomEffectManager::Instance().PlayEffect("Lamp", { prop.position.x + prop.effectOffset.x, prop.position.y + prop.effectOffset.y, prop.position.z + prop.effectOffset.z }, { 0, 0, 0 }, { 1, 1, 1 });
		}

		CustomEffectManager::Instance().SetPosition(prop.LampLight, { prop.position.x + prop.effectOffset.x, prop.position.y + prop.effectOffset.y, prop.position.z + prop.effectOffset.z });
	}
}

// 描画処理
void Stage::Render(const RenderContext& rc, ModelRenderer* renderer)
{
	if (drawStage)
	{
		// ステージを描画する
		renderer->Draw(ShaderId::PBR, stage);

		// 装飾物を描画する
		for (auto& p : props)
		{
			renderer->Draw(ShaderId::PBR, p.model);
		}
	}
}

// デバックプリミティブ描画
void Stage::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	if(drawArea)
	renderer->DrawCylinder(areaPosition, areaRadius, areaHeight, { 1.0f, 0.5f, 0.5f, 1.0f });
}

// GUI描画
void Stage::DrawGUI()
{
	// F8でステージパラメーターの表示をON/OFF
	static bool wasF8DownLastFrame = false;
	bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
	if (f8Down && !wasF8DownLastFrame)
	{
		showStageParameter = !showStageParameter;
	}
	wasF8DownLastFrame = f8Down;

	if (showStageParameter)
	{
		ImGui::Begin(u8"ステージ");
		ImGui::Checkbox(u8"ステージの表示", &drawStage);
		ImGui::Checkbox(u8"エリアの表示", &drawArea);
		ImGui::Separator();

		ImGui::DragFloat3(u8"エリアの位置", &areaPosition.x, 0.1f);
		ImGui::DragFloat(u8"エリアの半径", &areaRadius, 0.1f);
		ImGui::DragFloat(u8"エリアの高さ", &areaHeight, 0.1f);
		ImGui::Separator();

		ImGui::Text(u8"装飾");
		if (ImGui::Button(u8"保存"))     SaveProps(PropsFilePath);
		ImGui::SameLine();
		if (ImGui::Button(u8"再読み込み")) LoadProps(PropsFilePath);
		ImGui::Separator();

		if (ImGui::Button(u8"追加"))
		{
			char filePath[MAX_PATH] = { 0 };
			HWND hwnd = Graphics::Instance().GetWindowHandle();
			if (Dialog::OpenFileName(filePath, MAX_PATH,
				"Model File\0*.gltf;*.glb;*.fbx\0All Files\0*.*\0\0",
				"装飾モデルを選択", hwnd) == DialogResult::OK)
			{
				std::string path = filePath;
				size_t pos = path.find("Data");
				if (pos != std::string::npos) path = path.substr(pos); // 相対パスにする

				if (AddProp(path))
					selectedProp = static_cast<int>(props.size()) - 1;
			}
		}

		// 一覧
		if (ImGui::ListBoxHeader(u8"##props"))
		{
			for (int i = 0; i < static_cast<int>(props.size()); ++i)
			{
				std::string label = std::to_string(i) + ": "
					+ props[i].name.c_str();

				ImGui::PushID(i);
				if (ImGui::Selectable(label.c_str(), selectedProp == i))
					selectedProp = i;
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", props[i].path.c_str()); // フルパスはツールチップで
				ImGui::PopID();
			}
			ImGui::ListBoxFooter();
		}

		// 選択中の編集
		if (selectedProp >= 0 && selectedProp < static_cast<int>(props.size()))
		{
			StageProp& prop = props[selectedProp];

			char nameBuffer[128];
			strcpy_s(nameBuffer, prop.name.c_str());
			if (ImGui::InputText(u8"名前", nameBuffer, sizeof(nameBuffer)))
			{
				prop.name = nameBuffer;
			}
			ImGui::Separator();

			ImGui::DragFloat3(u8"位置", &prop.position.x, 0.1f);
			DirectX::XMFLOAT3 deg = {
				DirectX::XMConvertToDegrees(prop.angle.x),
				DirectX::XMConvertToDegrees(prop.angle.y),
				DirectX::XMConvertToDegrees(prop.angle.z) 
			};
			if (ImGui::DragFloat3(u8"回転", &deg.x, 1.0f))
			{
				prop.angle = {
				DirectX::XMConvertToRadians(deg.x),
				DirectX::XMConvertToRadians(deg.y),
				DirectX::XMConvertToRadians(deg.z) };
			}
			ImGui::DragFloat3(u8"スケール", &prop.scale.x, 0.001f);
			ImGui::Separator();

			ImGui::DragFloat(u8"回転速度", &prop.rotateSpeed, 1.0f);
			ImGui::DragFloat(u8"上下の振れ幅", &prop.bobHeight, 0.001f);
			ImGui::DragFloat(u8"上下の速さ", &prop.bobSpeed, 0.001f);
			ImGui::Separator();

			ImGui::DragFloat3(u8"ランプエフェクトオフセット", &prop.effectOffset.x, 0.1f);

			ImGui::Checkbox(u8"当たり判定", &prop.hasCollision);

			if (ImGui::Button(u8"削除"))
			{
				props.erase(props.begin() + selectedProp);
				selectedProp = -1;
			}
		}

		ImGui::End();
	}
}

// 行列を作る
DirectX::XMFLOAT4X4 Stage::MakeTransform(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& angle, const DirectX::XMFLOAT3& scale)
{
	//スケール行列作成
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);

	//回転行列を作成
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(angle.x, angle.y, angle.z);

	//位置行列を作成
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

	//3つの行列を組み合わせ,ワールド行列を作成
	DirectX::XMMATRIX W = S * R * T;

	//計算したワールド行列を取り出す
	DirectX::XMFLOAT4X4 transform;
	DirectX::XMStoreFloat4x4(&transform, W);

	return transform;
}

// 当たり判定を作り直す
void Stage::BuildCollision()
{
	triangles.clear();

	// 行列を作る
	transform = MakeTransform(position, angle, scale);
	AddCollisionTriangles(stage.get(), transform); // ステージ本体の三角形を集める

	for (StageProp& p : props)
	{
		if (!p.hasCollision) continue;
		p.transform = MakeTransform(p.position, p.angle, p.scale);
		AddCollisionTriangles(p.model.get(), p.transform); // 装飾の三角形を集める
	}

	BuildCollisionAreas();
}

// 三角形を集める
void Stage::AddCollisionTriangles(Model* model, const DirectX::XMFLOAT4X4& transform)
{
	// 先にモデルに行列を反映させる
	model->UpdateTransform(transform);

	// 	// 頂点データをワールド空間変換し、三角形データを作成
	for (const Model::Mesh& mesh : model->GetMeshes())
	{
		DirectX::XMMATRIX WorldTransform = DirectX::XMLoadFloat4x4(&mesh.node->worldTransform);
		for (size_t i = 0; i < mesh.indices.size(); i += 3)
		{
			// 頂点データをワールド空間変換
			uint32_t a = mesh.indices.at(i + 0);
			uint32_t b = mesh.indices.at(i + 1);
			uint32_t c = mesh.indices.at(i + 2);
			DirectX::XMVECTOR A = DirectX::XMLoadFloat3(&mesh.vertices.at(a).position);
			DirectX::XMVECTOR B = DirectX::XMLoadFloat3(&mesh.vertices.at(b).position);
			DirectX::XMVECTOR C = DirectX::XMLoadFloat3(&mesh.vertices.at(c).position);
			DirectX::XMVECTOR Scale = DirectX::XMLoadFloat3(&scale);
			A = DirectX::XMVector3Transform(A, WorldTransform);
			B = DirectX::XMVector3Transform(B, WorldTransform);
			C = DirectX::XMVector3Transform(C, WorldTransform);

			// 法線べクトルを算出
			DirectX::XMVECTOR N = DirectX::XMVector3Cross(DirectX::XMVectorSubtract(B, A),
				DirectX::XMVectorSubtract(C, A));
			if (DirectX::XMVector3Equal(N, DirectX::XMVectorZero()))
			{
				// 面を構成できない場合は除外
				continue;
			}
			N = DirectX::XMVector3Normalize(N);

			// 三角形データを格納
			Collision::Triangle& triangle = triangles.emplace_back();
			DirectX::XMStoreFloat3(&triangle.positions[0], A);
			DirectX::XMStoreFloat3(&triangle.positions[1], B);
			DirectX::XMStoreFloat3(&triangle.positions[2], C);
			DirectX::XMStoreFloat3(&triangle.normal, N);

			// 箱に入る最小点と最大点を保存する
			DirectX::XMStoreFloat3(&triangle.boundsMin, DirectX::XMVectorMin(A, DirectX::XMVectorMin(B, C)));
			DirectX::XMStoreFloat3(&triangle.boundsMax, DirectX::XMVectorMax(A, DirectX::XMVectorMax(B, C)));
		}
	}
}

// エリアと分割を作る
void Stage::BuildCollisionAreas()
{
	DirectX::XMVECTOR VolumeMin = DirectX::XMVectorReplicate(FLT_MAX);
	DirectX::XMVECTOR VolumeMax = DirectX::XMVectorReplicate(-FLT_MAX);

	// 三角形のループ
	for (const Collision::Triangle& t : triangles)
	{
		VolumeMin = DirectX::XMVectorMin(VolumeMin, DirectX::XMLoadFloat3(&t.boundsMin));
		VolumeMax = DirectX::XMVectorMax(VolumeMax, DirectX::XMLoadFloat3(&t.boundsMax));
	}

	// モデル全体のAABB
	DirectX::XMStoreFloat3(&volumeMin, VolumeMin);
	DirectX::XMStoreFloat3(&volumeMax, VolumeMax);

	areas.clear();

	// モデル全体のAABBをXZ平面で cellSize ごとに分けてエリアを作る
	{
		const int cellSize = 32;
		for (float x = volumeMin.x; x < volumeMax.x; x += cellSize)
		{
			for (float z = volumeMin.z; z < volumeMax.z; z += cellSize)
			{
				// エリアのAABB
				Collision::Area& area = areas.emplace_back();
				DirectX::XMFLOAT3 min = { x, volumeMin.y, z };
				DirectX::XMFLOAT3 max = { x + cellSize, volumeMax.y, z + cellSize };

				area.boundingBox.Center.x = (min.x + max.x) * 0.5f;
				area.boundingBox.Center.y = (min.y + max.y) * 0.5f;
				area.boundingBox.Center.z = (min.z + max.z) * 0.5f;

				area.boundingBox.Extents.x = (max.x - min.x) * 0.5f;
				area.boundingBox.Extents.y = (max.y - min.y) * 0.5f;
				area.boundingBox.Extents.z = (max.z - min.z) * 0.5f;

				// このエリアに重なる三角形を登録
				for (int i = 0; i < static_cast<int>(triangles.size()); ++i)
				{
					Collision::Triangle& tri = triangles[i];
					DirectX::XMFLOAT3 points[3] = { tri.positions[0], tri.positions[1], tri.positions[2] };
					DirectX::BoundingBox triangleBB;
					DirectX::BoundingBox::CreateFromPoints(triangleBB, 3, points, sizeof(DirectX::XMFLOAT3));

					if (area.boundingBox.Intersects(triangleBB))
					{
						area.triangleIndices.push_back(i);
					}
				}
			}
		}
	}
}

// 装飾の追加
Stage::StageProp* Stage::AddProp(const std::string& path)
{
	// パス違いでModelのコンストラクタが落ちないようにチェック
	if (!std::filesystem::exists(path))
	{
		return nullptr;
	}

	ID3D11Device* device = Graphics::Instance().GetDevice();

	StageProp& prop = props.emplace_back();
	prop.path = path;
	prop.model = std::make_shared<Model>(device, path.c_str());
	prop.transform = MakeTransform(prop.position, prop.angle, prop.scale);

	return &prop;
}

void Stage::SaveProps(const std::string& path)
{
	using json = nlohmann::json;
	json j;
	j["props"] = json::array();

	for (const StageProp& p : props)
	{
		j["props"].push_back({
			{ "path",      p.path },
			{ "name",      p.name },
			{ "position",  { p.position.x, p.position.y, p.position.z } },
			{ "angle",     { p.angle.x,    p.angle.y,    p.angle.z } },
			{ "scale",     { p.scale.x,    p.scale.y,    p.scale.z } },
			{ "rotateSpeed",      p.rotateSpeed },
			{ "bobHeight",      p.bobHeight },
			{ "bobSpeed",      p.bobSpeed },
			{ "collision", p.hasCollision },
			{ "effectOffset",  { p.effectOffset.x, p.effectOffset.y, p.effectOffset.z } },
			});
	}

	// フォルダがないと ofstream は失敗するので作っておく
	std::filesystem::create_directories(std::filesystem::path(path).parent_path());

	std::ofstream file(path);
	if (file.is_open())
	{
		file << j.dump(4);
	}
}

void Stage::LoadProps(const std::string& path)
{
	using json = nlohmann::json;
	std::ifstream file(path);
	if (!file.is_open()) return;   // 初回はファイルがないので何もしない

	json j;
	file >> j;
	if (!(j.find("props") != j.end()) || !j["props"].is_array()) return;

	props.clear();
	selectedProp = -1;

	for (const json& jp : j["props"])
	{
		StageProp* p = AddProp(jp.value("path", ""));
		if (!p) continue;          // モデルが見つからないものは飛ばす

		p->name = jp.value("name", "");

		auto readFloat3 = [&](const char* key, DirectX::XMFLOAT3& out)
			{
				if (jp.find(key) != jp.end())
					out = { jp[key][0].get<float>(), jp[key][1].get<float>(), jp[key][2].get<float>() };
			};
		readFloat3("position", p->position);
		readFloat3("effectOffset", p->effectOffset);
		readFloat3("angle", p->angle);
		readFloat3("scale", p->scale);
		p->hasCollision = jp.value("collision", false);
		p->rotateSpeed = jp.value("rotateSpeed", 0.0f);
		p->bobHeight = jp.value("bobHeight", 0.0f);
		p->bobSpeed = jp.value("bobSpeed", 0.0f);
		p->transform = MakeTransform(p->position, p->angle, p->scale);
	}
}
