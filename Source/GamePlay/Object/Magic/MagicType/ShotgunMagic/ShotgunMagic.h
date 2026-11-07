#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicInfor.h"
#include <fstream>
#include <random>


class ShotgunMagic
{
public:
	struct ShotgunMagicConfig : public MagicInfo
	{
		float spreadAngle = 12.0f; // ŠgU—¦
		int pelletCount = 8; // “¯‚Éo‚Ä‚­‚é’e‚Ì”
		float falloffStart = 0.0f;   // ‚±‚±‚Ü‚Å‚Í100%
		float falloffEnd = 0.0f;     // ‚±‚±‚ÅÅ’á‚Ì”{—¦‚É‚È‚é
		float falloffMinRate = 1.0f; // Å’á‚Ì”{—¦

		void Save() const
		{
			nlohmann::json j;
			SaveBase(j); // e‚ğ•Û‘¶

			j["spreadAngle"] = spreadAngle;
			j["pelletCount"] = pelletCount;
			j["falloffStart"] = falloffStart;
			j["falloffEnd"] = falloffEnd;
			j["falloffMinRate"] = falloffMinRate;

			std::ofstream file("Data/Json/Magic/ShotgunMagic.json");
			file << j.dump(4);
		}

		void Load()
		{
			std::ifstream file("Data/Json/Magic/ShotgunMagic.json");

			if (file.is_open())
			{
				nlohmann::json j;
				file >> j;

				LoadBase(j); // e‚ğ“Ç‚İ‚Ş

				if (j.find("spreadAngle") != j.end()) spreadAngle = j["spreadAngle"];
				if (j.find("pelletCount") != j.end()) pelletCount = j["pelletCount"];
				if (j.find("falloffStart") != j.end()) falloffStart = j["falloffStart"];
				if (j.find("falloffEnd") != j.end()) falloffEnd = j["falloffEnd"];
				if (j.find("falloffMinRate") != j.end()) falloffMinRate = j["falloffMinRate"];
			}
		}
	};

public:
	// ‰Šú‰»
	void Initialize();

	// ”­ËŠÖ”
	void Emit(MagicManager* manager, const MagicSpawnInfo& info, const ShotgunMagicConfig& config);

	// GUI•`‰æ
	void DrawGUI(ShotgunMagicConfig& config);

private:
	std::mt19937 rng{ std::random_device{}() };
};