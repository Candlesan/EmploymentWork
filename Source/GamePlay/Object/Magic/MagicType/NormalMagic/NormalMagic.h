#pragma once
#include "GamePlay/Object/Magic/MagicManager.h"
#include "GamePlay/Object/Magic/MagicInfor.h"
#include <fstream>

class NormalMagic
{
public:
    struct NormalMagicConfig : public MagicInfo
    {
        float normalDamage = 10.0f;         // 通常攻撃のダメージ
        float guardDamage = 0.5;            // 防御魔法へのダメージ
        float chargeDamage = 20.0f;         // チャージ時のダメージ

        float normalRadius = 0.5f;          // 通常時のレーザーの太さ
        float chargeRadius = 1.0f;          // チャージ時のレーザーの太さ

        float normalMagicCost = 10.0f;      // 通常の魔力消費量
        float chargeMagicCost = 20.0f;      // チャージ時の魔力消費量
        float chargeHoldMagicCost = 1.0f;   // 最大チャージ維持中の魔力消費量

        float range = 100.0f;		        // 射程距離

        float chargeTimeThreshold = 3.0;   // レーザーへ変化する長押し時間

        DirectX::XMFLOAT3 offsetPosition = { 0.0f, 0.0f, 0.0f }; // 発生位置のズレ
        DirectX::XMFLOAT3 offsetAngle = { 0.0f, 0.0f, 0.0f };    // 回転のズレ
        DirectX::XMFLOAT3 offsetScale = { 1.0f, 1.0f, 1.0f };    // スケール倍率

        void Save() const
        {
            nlohmann::json j;
            SaveBase(j); // 親(共通部分)を保存

            j["normalDamage"] = normalDamage;
            j["guardDamage"] = guardDamage;
            j["chargeDamage"] = chargeDamage;

            j["normalRadius"] = normalRadius;
            j["chargeRadius"] = chargeRadius;

            j["normalMagicCost"] = normalMagicCost;
            j["chargeMagicCost"] = chargeMagicCost;
            j["chargeHoldMagicCost"] = chargeHoldMagicCost;

            j["range"] = range;

            j["chargeTimeThreshold"] = chargeTimeThreshold;

            j["offsetPosition"] = { offsetPosition.x, offsetPosition.y, offsetPosition.z };
            j["offsetAngle"] = { offsetAngle.x, offsetAngle.y, offsetAngle.z };
            j["offsetScale"] = { offsetScale.x, offsetScale.y, offsetScale.z };

            std::ofstream file("Data/Json/Magic/NormalMagic.json");
            file << j.dump(4);
        }

        void Load()
        {
            std::ifstream file("Data/Json/Magic/NormalMagic.json");

            if (file.is_open())
            {
                nlohmann::json j;
                file >> j;

                LoadBase(j); // 親を読み込む
                if (j.find("normalDamage") != j.end()) normalDamage = j["normalDamage"];
                if (j.find("guardDamage") != j.end()) guardDamage = j["guardDamage"];
                if (j.find("chargeDamage") != j.end()) chargeDamage = j["chargeDamage"];

                if (j.find("normalRadius") != j.end()) normalRadius = j["normalRadius"];
                if (j.find("chargeRadius") != j.end()) chargeRadius = j["chargeRadius"];

                if (j.find("normalMagicCost") != j.end()) normalMagicCost = j["normalMagicCost"];
                if (j.find("chargeMagicCost") != j.end()) chargeMagicCost = j["chargeMagicCost"];
                if (j.find("chargeHoldMagicCost") != j.end()) chargeHoldMagicCost = j["chargeHoldMagicCost"];

                if (j.find("range") != j.end()) range = j["range"];

                if (j.find("chargeTimeThreshold") != j.end()) chargeTimeThreshold = j["chargeTimeThreshold"];

                if (j.find("offsetPosition") != j.end()) offsetPosition = { j["offsetPosition"][0], j["offsetPosition"][1], j["offsetPosition"][2] };
                if (j.find("offsetAngle") != j.end()) offsetAngle = { j["offsetAngle"][0], j["offsetAngle"][1], j["offsetAngle"][2] };
                if (j.find("offsetScale") != j.end()) offsetScale = { j["offsetScale"][0], j["offsetScale"][1], j["offsetScale"][2] };
            }
        }
    };

    // 初期化
    void Initialize();

    // 更新処理
    void Update(float elapsedTime, Character* owner, const NormalMagicConfig& config, bool isButtonHeld);

    // 発射関数
    void Emit(MagicManager* manager, const MagicSpawnInfo& info, const NormalMagicConfig& config);

    // GUI描画 (config を参照で受け取る！)
    void DrawGUI(NormalMagicConfig& config);

    bool IsFullCharged() const { return isFullCharged; }
    bool IsChargeing() const { return isCharging; }

    void ResetCharge();

private:
    std::string normalMagic = "Laser"; // 読み込むエフェクト名
    std::string chargeMagic = "Charge_Laser"; // 読み込むエフェクト名

    float chargeTimer = 0.0f; // 現在のチャージした時間
    bool isCharging = false; // チャージ中か
    bool isFullCharged = false; // フルチャージか
};