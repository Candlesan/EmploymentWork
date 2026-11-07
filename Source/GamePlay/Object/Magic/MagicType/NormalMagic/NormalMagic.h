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
        float chargeDamage = 20.0f;         // チャージ時のダメージ

        float normalRadius = 0.5f;          // 通常時のレーザーの太さ
        float chargeRadius = 1.0f;          // チャージ時のレーザーの太さ

        float normalMagicCost = 10.0f;      // 通常の魔力消費量
        float chargeMagicCost = 20.0f;      // チャージ時の魔力消費量
        float chargeHoldMagicCost = 1.0f;   // 最大チャージ維持中の魔力消費量

        float range = 100.0f;		        // 射程距離

        float chargeTimeThreshold = 3.0;   // レーザーへ変化する長押し時間

        DirectX::XMFLOAT3 laserOffsetPosition = { 0.0f, 0.0f, 0.0f }; // 発生位置のズレ
        DirectX::XMFLOAT3 laserOffsetAngle = { 0.0f, 0.0f, 0.0f };    // 回転のズレ
        DirectX::XMFLOAT3 laserOffsetScale = { 1.0f, 1.0f, 1.0f };    // スケール倍率

        DirectX::XMFLOAT3 laserChargeOffsetPosition = { 0.0f, 0.0f, 0.0f }; // 発生位置のズレ
        DirectX::XMFLOAT3 laserChargeOffsetAngle = { 0.0f, 0.0f, 0.0f };    // 回転のズレ
        DirectX::XMFLOAT3 laserChargeOffsetScale = { 1.0f, 1.0f, 1.0f };    // スケール倍率

        DirectX::XMFLOAT3 circleOffsetPosition = { 0.0f, 0.0f, 0.0f }; // 発生位置のズレ
        DirectX::XMFLOAT3 circleOffsetAngle = { 0.0f, 0.0f, 0.0f };    // 回転のズレ
        DirectX::XMFLOAT3 circleOffsetScale = { 1.0f, 1.0f, 1.0f };    // スケール倍率

        void Save() const
        {
            nlohmann::json j;
            SaveBase(j); // 親(共通部分)を保存

            j["normalDamage"] = normalDamage;
            j["chargeDamage"] = chargeDamage;

            j["normalRadius"] = normalRadius;
            j["chargeRadius"] = chargeRadius;

            j["normalMagicCost"] = normalMagicCost;
            j["chargeMagicCost"] = chargeMagicCost;
            j["chargeHoldMagicCost"] = chargeHoldMagicCost;

            j["range"] = range;

            j["chargeTimeThreshold"] = chargeTimeThreshold;

            j["laserOffsetPosition"] = { laserOffsetPosition.x, laserOffsetPosition.y, laserOffsetPosition.z };
            j["laserOffsetAngle"] = { laserOffsetAngle.x, laserOffsetAngle.y, laserOffsetAngle.z };
            j["laserOffsetScale"] = { laserOffsetScale.x, laserOffsetScale.y, laserOffsetScale.z };

            j["laserChargeOffsetPosition"] = { laserChargeOffsetPosition.x, laserChargeOffsetPosition.y, laserChargeOffsetPosition.z };
            j["laserChargeOffsetAngle"] = { laserChargeOffsetAngle.x, laserChargeOffsetAngle.y, laserChargeOffsetAngle.z };
            j["laserChargeOffsetScale"] = { laserChargeOffsetScale.x, laserChargeOffsetScale.y, laserChargeOffsetScale.z };

            j["circleOffsetPosition"] = { circleOffsetPosition.x, circleOffsetPosition.y, circleOffsetPosition.z };
            j["circleOffsetAngle"] = { circleOffsetAngle.x, circleOffsetAngle.y, circleOffsetAngle.z };
            j["circleOffsetScale"] = { circleOffsetScale.x, circleOffsetScale.y, circleOffsetScale.z };

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
                if (j.find("chargeDamage") != j.end()) chargeDamage = j["chargeDamage"];

                if (j.find("normalRadius") != j.end()) normalRadius = j["normalRadius"];
                if (j.find("chargeRadius") != j.end()) chargeRadius = j["chargeRadius"];

                if (j.find("normalMagicCost") != j.end()) normalMagicCost = j["normalMagicCost"];
                if (j.find("chargeMagicCost") != j.end()) chargeMagicCost = j["chargeMagicCost"];
                if (j.find("chargeHoldMagicCost") != j.end()) chargeHoldMagicCost = j["chargeHoldMagicCost"];

                if (j.find("range") != j.end()) range = j["range"];

                if (j.find("chargeTimeThreshold") != j.end()) chargeTimeThreshold = j["chargeTimeThreshold"];

                if (j.find("laserOffsetPosition") != j.end()) laserOffsetPosition = { j["laserOffsetPosition"][0], j["laserOffsetPosition"][1], j["laserOffsetPosition"][2] };
                if (j.find("laserOffsetAngle") != j.end()) laserOffsetAngle = { j["laserOffsetAngle"][0], j["laserOffsetAngle"][1], j["laserOffsetAngle"][2] };
                if (j.find("laserOffsetScale") != j.end()) laserOffsetScale = { j["laserOffsetScale"][0], j["laserOffsetScale"][1], j["laserOffsetScale"][2] };

                if (j.find("laserChargeOffsetPosition") != j.end()) laserChargeOffsetPosition = { j["laserChargeOffsetPosition"][0], j["laserChargeOffsetPosition"][1], j["laserChargeOffsetPosition"][2] };
                if (j.find("laserChargeOffsetAngle") != j.end()) laserChargeOffsetAngle = { j["laserChargeOffsetAngle"][0], j["laserChargeOffsetAngle"][1], j["laserChargeOffsetAngle"][2] };
                if (j.find("laserChargeOffsetScale") != j.end()) laserChargeOffsetScale = { j["laserChargeOffsetScale"][0], j["laserChargeOffsetScale"][1], j["laserChargeOffsetScale"][2] };

                if (j.find("circleOffsetPosition") != j.end()) circleOffsetPosition = { j["circleOffsetPosition"][0], j["circleOffsetPosition"][1], j["circleOffsetPosition"][2] };
                if (j.find("circleOffsetAngle") != j.end()) circleOffsetAngle = { j["circleOffsetAngle"][0], j["circleOffsetAngle"][1], j["circleOffsetAngle"][2] };
                if (j.find("circleOffsetScale") != j.end()) circleOffsetScale = { j["circleOffsetScale"][0], j["circleOffsetScale"][1], j["circleOffsetScale"][2] };
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
    std::string magicCircleFast = "magicCircleFast";
    std::string magicCircleLong = "magicCircleLong";

    float chargeTimer = 0.0f; // 現在のチャージした時間
    bool isCharging = false; // チャージ中か
    bool isFullCharged = false; // フルチャージか
};