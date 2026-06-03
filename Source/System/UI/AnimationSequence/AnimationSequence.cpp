#include "System/UI/AnimationSequence/AnimationSequence.h"
#include "json.hpp"
#include <fstream>
#include <algorithm>
#include <iostream>

// コンストラクタ
AnimationSequencer::AnimationSequencer(const std::string& jsonPath)
    : jsonPath(jsonPath)
{
    Load(jsonPath);

    // 初回の更新日時を記録
    if (!jsonPath.empty() && std::filesystem::exists(jsonPath))
    {
        lastWriteTime = std::filesystem::last_write_time(jsonPath);
    }
}

// 更新処理（友達のシステムそのまま！）
void AnimationSequencer::Update(const std::string& animName, float animTime)
{
#if _DEBUG
    CheckAndReload();
#endif
    // アニメーションが切り替わったらリセット
    if (animName != currentAnimName)
    {
        currentAnimName = animName;
        prevAnimTime = 0.0f;
        firedEvents.clear();
        activeFlags.clear();
        currentSpeed = 1.0f;
    }

    // アニメーションデータを検索
    AnimationData* data = FindAnimationData(animName);
    if (!data)
    {
        prevAnimTime = animTime;
        return;
    }

    // イベント判定（前フレームと今フレームの間を通過したか）
    firedEvents.clear();
    for (Event& e : data->events)
    {
        if (prevAnimTime < e.time && animTime >= e.time)
        {
            firedEvents[e.name] = true;
            e.played = true; // 君のシステム用の再生フラグもONにする
        }
    }

    // フラグ判定（現在時刻が範囲内か）
    activeFlags.clear();
    for (const Range& r : data->ranges)
    {
        if (animTime >= r.start && animTime <= r.end)
        {
            activeFlags[r.name] = true;
        }
    }

    // 速度計算（キーフレーム間を線形補間）
    currentSpeed = 1.0f;
    if (!data->speedKeys.empty())
    {
        if (data->speedKeys.size() == 1)
        {
            currentSpeed = data->speedKeys[0].speed;
        }
        else
        {
            if (animTime <= data->speedKeys.front().time)
            {
                currentSpeed = data->speedKeys.front().speed;
            }
            else if (animTime >= data->speedKeys.back().time)
            {
                currentSpeed = data->speedKeys.back().speed;
            }
            else
            {
                for (size_t i = 0; i < data->speedKeys.size() - 1; i++)
                {
                    const SpeedKey& a = data->speedKeys[i];
                    const SpeedKey& b = data->speedKeys[i + 1];
                    if (animTime >= a.time && animTime <= b.time)
                    {
                        float t = (animTime - a.time) / (b.time - a.time);
                        currentSpeed = a.speed + (b.speed - a.speed) * t;
                        break;
                    }
                }
            }
        }
    }

    // 今フレームの時刻を保存
    prevAnimTime = animTime;
}

// 結果取得用ゲッター
bool AnimationSequencer::GetEvent(const std::string& eventName) const
{
    auto it = firedEvents.find(eventName);
    return it != firedEvents.end() && it->second;
}

bool AnimationSequencer::GetRange(const std::string& flagName) const
{
    auto it = activeFlags.find(flagName);
    return it != activeFlags.end() && it->second;
}

float AnimationSequencer::GetSpeed() const
{
    return currentSpeed;
}

// アニメーションデータ検索
AnimationSequencer::AnimationData* AnimationSequencer::FindAnimationData(const std::string& animName)
{
    for (AnimationData& data : animations)
    {
        if (data.name == animName)
        {
            return &data;
        }
    }
    return nullptr;
}

// ★君のnlohmann::jsonを使った最強のSave処理
void AnimationSequencer::Save(const std::string& filepath)
{
    // フォルダが無ければ作る（君の元のコードの賢い処理を残したで！）
    std::filesystem::path path(filepath);
    if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
        std::filesystem::create_directories(path.parent_path());
    }

    nlohmann::json root;
    root["animations"] = nlohmann::json::array();

    for (auto& anim : animations)
    {
        nlohmann::json jAnim;
        jAnim["name"] = anim.name;

        // Events
        jAnim["events"] = nlohmann::json::array();
        for (auto& e : anim.events)
        {
            jAnim["events"].push_back({
                {"name", e.name},
                {"time", e.time},
                {"type", (int)e.type},
                {"soundName", e.soundName},
                {"effectName", e.effectName}
                });
        }

        // Ranges (君の当たり判定パラメーターを全部保存！)
        jAnim["ranges"] = nlohmann::json::array();
        for (auto& r : anim.ranges)
        {
            jAnim["ranges"].push_back({
                {"name", r.name},
                {"start", r.start},
                {"end", r.end},
                {"hand", (int)r.hand},
                {"boneName", r.boneName},
                {"sphereRadius", r.sphereRadius},
                {"sphereOffset", {r.sphereOffset.x, r.sphereOffset.y, r.sphereOffset.z}},
                {"damageRate", r.damageRate},
                {"poiseRate", r.poiseRate},
                {"invincible", r.invincible}
                });
        }

        // SpeedKeys
        jAnim["speedKeys"] = nlohmann::json::array();
        for (auto& sk : anim.speedKeys)
        {
            jAnim["speedKeys"].push_back({
                {"time", sk.time},
                {"speed", sk.speed}
                });
        }

        root["animations"].push_back(jAnim);
    }

    std::ofstream file(filepath);
    if (file.is_open()) file << root.dump(4);
}

// ★君のnlohmann::jsonを使った最強のLoad処理
void AnimationSequencer::Load(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    try
    {
        nlohmann::json root;
        file >> root;
        animations.clear();

        if (root.count("animations") > 0)
        {
            for (auto& jAnim : root["animations"])
            {
                AnimationData data;
                data.name = jAnim.value("name", "");

                // Events
                if (jAnim.count("events") > 0)
                {
                    for (auto& je : jAnim["events"])
                    {
                        Event e;
                        e.name = je.value("name", "NewEvent");
                        e.time = je.value("time", 0.0f);
                        e.type = (EventType)je.value("type", 0);
                        e.soundName = je.value("soundName", "");
                        e.effectName = je.value("effectName", "");
                        e.played = false; // 初期化
                        data.events.push_back(e);
                    }
                }

                // Ranges (君の当たり判定パラメーターを全部復元！)
                if (jAnim.count("ranges") > 0)
                {
                    for (auto& jr : jAnim["ranges"])
                    {
                        Range r;
                        r.name = jr.value("name", "NewRange");
                        r.start = jr.value("start", 0.0f);
                        r.end = jr.value("end", 0.0f);
                        r.hand = (HandType)jr.value("hand", 0);
                        r.boneName = jr.value("boneName", "");
                        r.sphereRadius = jr.value("sphereRadius", 0.5f);

                        if (jr.count("sphereOffset") > 0 && jr["sphereOffset"].is_array() && jr["sphereOffset"].size() == 3)
                        {
                            r.sphereOffset.x = jr["sphereOffset"][0];
                            r.sphereOffset.y = jr["sphereOffset"][1];
                            r.sphereOffset.z = jr["sphereOffset"][2];
                        }

                        r.damageRate = jr.value("damageRate", 1.0f);
                        r.poiseRate = jr.value("poiseRate", 1.0f);
                        r.invincible = jr.value("invincible", 0.3f);
                        data.ranges.push_back(r);
                    }
                }

                // SpeedKeys
                if (jAnim.count("speedKeys") > 0)
                {
                    for (auto& jsk : jAnim["speedKeys"])
                    {
                        SpeedKey sk;
                        sk.time = jsk.value("time", 0.0f);
                        sk.speed = jsk.value("speed", 1.0f);
                        data.speedKeys.push_back(sk);
                    }
                }

                animations.push_back(data);
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "JSON Load Error (" << filepath << "): " << e.what() << std::endl;
    }
}

// ファイルの更新日時を確認してリロード（友達の便利機能）
void AnimationSequencer::CheckAndReload()
{
    if (jsonPath.empty()) return;
    if (!std::filesystem::exists(jsonPath)) return;

    try
    {
        auto writeTime = std::filesystem::last_write_time(jsonPath);
        if (writeTime != lastWriteTime)
        {
            lastWriteTime = writeTime;
            Load(jsonPath);
        }
    }
    catch (...) {}
}