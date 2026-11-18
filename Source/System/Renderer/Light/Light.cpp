#include "Light.h"
#include "json.hpp"

#include <fstream>
#include <filesystem>

using json = nlohmann::json;

// XMFLOAT3 を JSON の配列 [x, y, z] に変換する関数
static json ToJson(const DirectX::XMFLOAT3& v) { return { v.x, v.y, v.z }; }
static void ReadFloat3(const json& j, const char* key, DirectX::XMFLOAT3& out)
{
	if ((j.find(key) != j.end()) && j[key].is_array() && j[key].size() == 3)
		out = { j[key][0].get<float>(), j[key][1].get<float>(), j[key][2].get<float>() };
}


void LightManager::Save(const std::string& path) const
{
	json j;
	j["points"] = json::array();
	for (const PointLight& p : pointLights)
	{
		j["points"].push_back({
			{"position", ToJson(p.position)},
			{"color", ToJson(p.color)},
			{"intensity", p.intensity},
			{"range", p.range},
			{"enabled", p.enabled},
		});
	}
	j["spots"] = json::array();
	for (const SpotLight& p : spotLights)
	{
		j["spots"].push_back({
			{"position", ToJson(p.position)},
			{"direction", ToJson(p.direction)},
			{"color", ToJson(p.color)},
			{"intensity", p.intensity},
			{"range", p.range},
			{"innerAngle", p.innerAngle},
			{"range", p.range},
			{"enabled", p.enabled},
		});
	}

	std::ofstream file(path);
	if (file.is_open()) file << j.dump(4);
}

void LightManager::Load(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) return; // 初回は

	json j;
	file >> j;

	pointLights.clear();
	spotLights.clear();

	if ((j.find("points") != j.end()) && j["points"].is_array())
	{
		for (const json& jp : j["points"])
		{
			PointLight& p = pointLights.emplace_back();

			ReadFloat3(jp, "position", p.position);
			ReadFloat3(jp, "color", p.color);
			p.intensity = jp.value("intensity", p.intensity);
			p.range = jp.value("range", p.range);
			p.enabled = jp.value("enabled", p.enabled);
		}
	}
	if ((j.find("spots") != j.end()) && j["spots"].is_array())
	{
		for (const json& js : j["spots"])
		{
			SpotLight& s = spotLights.emplace_back();

			ReadFloat3(js, "position", s.position);
			ReadFloat3(js, "direction", s.direction);
			ReadFloat3(js, "color", s.color);
			s.intensity = js.value("intensity", s.intensity);
			s.range = js.value("range", s.range);
			s.innerAngle = js.value("innerAngle", s.innerAngle);
			s.outerAngle = js.value("outerAngle", s.outerAngle);
			s.enabled = js.value("enabled", s.enabled);
		}
	}
}

// ワイヤーフレーム描画
void LightManager::RenderDebugPrimitive(ShapeRenderer* renderer)
{
	// ポイントライト
	for (auto& p : pointLights)
	{
		if (!p.enabled) continue;
		DirectX::XMFLOAT4 c = { p.color.x, p.color.y, p.color.z, 1 };
		renderer->DrawSphere(p.position, 0.2f, c);      // 中心
		renderer->DrawSphere(p.position, p.range, c);   // 範囲
	}

	// スポットライト
	for (const SpotLight& s : spotLights)
	{
		if (!s.enabled) continue;
		DirectX::XMFLOAT4 c = { s.color.x, s.color.y, s.color.z, 1 };
		renderer->DrawSphere(s.position, 0.3f, c);      // 根元

		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&s.position);
		DirectX::XMVECTOR D = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&s.direction));
		const float step = s.range / 5.0f;              // 最後の点が range の位置に来る
		for (int i = 1; i <= 5; ++i)
		{
			DirectX::XMFLOAT3 q;
			DirectX::XMStoreFloat3(&q, DirectX::XMVectorAdd(P, DirectX::XMVectorScale(D, step * i)));
			renderer->DrawSphere(q, 0.08f, c);
		}
	}
}