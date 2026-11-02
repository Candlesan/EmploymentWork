#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicInfor.h"
#include <fstream>
#include <random>


class RapidMagic
{
public:
    struct RapidMagicConfig : public MagicInfo
    {
        float rapidInterval = 0.1; // 発射間隔
        float gaurdDamage = 2; // 防御魔法へのダメージ
        float diffusion = 0.0f; // 拡散
        float magicCost = 6.0f; // 魔力消費量
        
       DirectX::XMFLOAT3 spawnOffsets[6] = {
            { -1.0f,  1.0f,   0.0f }, // 左上
            { -2.0f,  0.5f,   0.0f }, // 左中
            { -1.0f,  0.25f,  0.0f }, // 左下
            {  1.0f,  1.0f,   0.0f }, // 右上
            {  2.0f,  0.5f,   0.0f }, // 右中
            {  1.0f,  0.25f,  0.0f }  // 右下    
        };

        DirectX::XMFLOAT3 offsetPosition = { 0.0f, 0.0f, 0.0f }; // 発生位置のズレ
        DirectX::XMFLOAT3 offsetAngle = { 0.0f, 0.0f, 0.0f };    // 回転のズレ
        DirectX::XMFLOAT3 offsetScale = { 1.0f, 1.0f, 1.0f };    // スケール倍率

        void Save() const
        {
            nlohmann::json j;
            SaveBase(j); // 親(共通部分)を保存

            j["rapidInterval"] = rapidInterval;
            j["magicCost"] = magicCost;
            j["diffusion"] = diffusion;
            j["gaurdDamage"] = gaurdDamage;
            j["offsetPosition"] = { offsetPosition.x, offsetPosition.y, offsetPosition.z };
            j["offsetAngle"] = { offsetAngle.x, offsetAngle.y, offsetAngle.z };
            j["offsetScale"] = { offsetScale.x, offsetScale.y, offsetScale.z };
            j["spawnOffsets"] = {
                { spawnOffsets[0].x, spawnOffsets[0].y, spawnOffsets[0].z },
                { spawnOffsets[1].x, spawnOffsets[1].y, spawnOffsets[1].z },
                { spawnOffsets[2].x, spawnOffsets[2].y, spawnOffsets[2].z },
                { spawnOffsets[3].x, spawnOffsets[3].y, spawnOffsets[3].z },
                { spawnOffsets[4].x, spawnOffsets[4].y, spawnOffsets[4].z },
                { spawnOffsets[5].x, spawnOffsets[5].y, spawnOffsets[5].z }
            };


            std::ofstream file("Data/Json/Magic/RapidlMagic.json");
            file << j.dump(4);
        }

        void Load()
        {
            std::ifstream file("Data/Json/Magic/RapidlMagic.json");

            if (file.is_open())
            {
                nlohmann::json j;
                file >> j;

                LoadBase(j); // 親を読み込む
                if (j.find("rapidInterval") != j.end()) rapidInterval = j["rapidInterval"];
                if (j.find("magicCost") != j.end()) magicCost = j["magicCost"];
                if (j.find("diffusion") != j.end()) diffusion = j["diffusion"];
                if (j.find("gaurdDamage") != j.end()) gaurdDamage = j["gaurdDamage"];
                if (j.find("offsetPosition") != j.end()) offsetPosition = { j["offsetPosition"][0], j["offsetPosition"][1], j["offsetPosition"][2] };
                if (j.find("offsetAngle") != j.end()) offsetAngle = { j["offsetAngle"][0], j["offsetAngle"][1], j["offsetAngle"][2] };
                if (j.find("offsetScale") != j.end()) offsetScale = { j["offsetScale"][0], j["offsetScale"][1], j["offsetScale"][2] };
                if (j.find("spawnOffsets") != j.end()) {
                    for (int i = 0; i < 6; ++i) {
                        spawnOffsets[i] = {
                            j["spawnOffsets"][i][0],
                            j["spawnOffsets"][i][1],
                            j["spawnOffsets"][i][2]
                        };
                    }
                }

            }
        }
    };

public:
    // 初期化
    void Initialize();

    // 発射関数
    void Emit(MagicManager* manager, const MagicSpawnInfo& info, const RapidMagicConfig& config);

    // 更新処理
    void Update(float elapsedTime, MagicManager* manager, const MagicSpawnInfo& info, const RapidMagicConfig& config, bool isTriggered);

    // GUI描画 (config を参照で受け取る！)
    void DrawGUI(RapidMagicConfig& config);

   // エフェクト用のセッター
    void SetEffect(const std::string& effectName) { rapidMagic = effectName; }
    void SetHitEffect(const std::string& hitEffectName) { rapidHitEfc = hitEffectName; }

private:
    std::string rapidMagic = "RapidMagic"; // エフェクト本体
    std::string rapidHitEfc = "RapidHit"; // ヒットエフェクト
    int currentSpawnIndex = 0;
    float rapidTimer = 0.0f;
    std::mt19937 rng{ std::random_device{}() };
};