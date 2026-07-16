#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Character/Enemy/Enemy.h"
#include "GamePlay/Object/Magic/MagicInfor.h"
#include <fstream>


class WindBladeMagic
{
public:
	struct WindBladeConfig : public MagicInfo
	{
		float waitTimer = 0.16f;   // 入力から実際に発射されるまでのディレイ時間

        void Save() const
        {
            nlohmann::json j;
            SaveBase(j); // 親(共通部分)を保存

            j["waitTimer"] = waitTimer; // 自分(風の刃)専用の部分を保存

            std::ofstream file("Data/Json/Player/Magic/WindBlade.json");
            file << std::setw(4) << j << std::endl;
        }

        
        void Load()
        {
            std::ifstream file("Data/Json/Player/Magic/WindBlade.json");

            if (file.is_open()) 
            {
                nlohmann::json j;

                file >> j;

                LoadBase(j); // 親を読み込む
                if (j.find("waitTimer") != j.end()) waitTimer = j["waitTimer"];
            }
        }
	};

	// 球を生成する関数
	static void Emit(MagicManager* manager, const MagicSpawnInfo& info, const WindBladeConfig& config);
};

