#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicInfor.h"
#include <fstream>


class NormalMagic
{
public:
	struct  NormalMagicConfig : public MagicInfo
	{
		float normalDamage = 5.0f;  // 通常攻撃のダメージ
		float chargeDamage = 10.0f; // チャージ時のダメージ
		float normalRadius = 0.5f;  // 通常時のレーザーの太さ
		float chargeRadius = 1.0f;  // チャージ時のレーザーの太さ
		float range = 100;			// 射程距離
        float chargeTimeThreshold = 0.5f; // レーザーへ変化する長押し時間

        void Save() const
        {
            nlohmann::json j;
            SaveBase(j); // 親(共通部分)を保存

            j["normalDamage"] = normalDamage;
            j["chargeDamage"] = chargeDamage;
            j["normalRadius"] = normalRadius;
            j["chargeRadius"] = chargeRadius;
            j["range"] = range;
            j["chargeTimeThreshold"] = chargeTimeThreshold;

            std::ofstream file("Data/Json/Player/Magic/NormalMagic.json");
            file << j.dump(4);
        }


        void Load()
        {
            std::ifstream file("Data/Json/Player/Magic/NormalMagic.json");

            if (file.is_open())
            {
                nlohmann::json j;

                file >> j;

                LoadBase(j); // 親を読み込む
                if (j.find("normalDamage") != j.end()) normalDamage = j["normalDamage"];
                if (j.find("chargeDamage") != j.end()) chargeDamage = j["chargeDamage"];
                if (j.find("normalRadius") != j.end()) normalRadius = j["normalRadius"];
                if (j.find("chargeRadius") != j.end()) chargeRadius = j["chargeRadius"];
                if (j.find("range") != j.end()) range = j["range"];
                if (j.find("chargeTimeThreshold") != j.end()) chargeTimeThreshold = j["chargeTimeThreshold"];
            }
        }
	};

    // 発射関数
    static void Emit(MagicManager* manager, const MagicSpawnInfo& info, const NormalMagicConfig& config);
};