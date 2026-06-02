#pragma once
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"
#include "System/Resource/Model/Model.h"
#include "ImSequencer.h"
#include "json.hpp"
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

enum class TrackType {
    HitBox,
    Effect,
    Sound,
};

enum class HandType {
    None,
    RightHand,
    LeftHand,
    Both, // 両手攻撃
    Body, // 体当たりなど
};

struct AnimTrack {
    int start;
    int end;
    std::string label;
    unsigned int color;
    TrackType type;

    // 攻撃属性
    HandType hand = HandType::None;

    // エフェクト
    std::string effectName = "";
    bool effectPlayed = false;

    // サウンド
    std::string soundName = "";
    bool soundPlayed = false;

    // 球判定用（hand == Body のときに使う）
    std::string boneName = "";   // 追従するボーン名（空なら敵の position 基準）
    float sphereRadius = 0.5f;   // 球の半径
    DirectX::XMFLOAT3 sphereOffset = { 0, 0, 0 }; // ボーンからのオフセット

    // 攻撃系のパラメーター設定変数
    float damageRate = 0.0f;
    float poiseRate = 0.0f;
    float invincible = 0.3f;

    float GetStartSeconds() const { return start / 100.0f; }
    float GetEndSeconds()   const { return end / 100.0f; }
};

template<typename T>
class AnimationSequence : public ImSequencer::SequenceInterface
{
public:
    std::unordered_map<T, std::vector<AnimTrack>> attackData;
    T currentState; // 初期値は各Enumの0番など

    void SetModel(std::shared_ptr<Model> model) { this->model = model; }

    // --- ImSequencer オーバーライド群 ---

    int GetFrameMin() const override { return 0; }
    // アニメーションの秒数 * 100 を最大フレームとする例
    int GetFrameMax() const override {
        return (int)(GetAnimationLength(currentState) * 100.0f);
    }

    int GetItemCount() const override {
        auto it = attackData.find(currentState);
        return (it == attackData.end()) ? 0 : (int)it->second.size();
    }

    void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
        auto& tracks = attackData[currentState];
        auto& t = tracks[index];
        if (start) *start = &t.start;
        if (end)   *end = &t.end;
        if (color) *color = t.color;
        if (type)  *type = (int)t.type;
    }

    const char* GetItemLabel(int index) const override {
        auto it = attackData.find(currentState);
        if (it == attackData.end() || index >= it->second.size()) return "Unknown";
        return it->second[index].label.c_str();
    }

    void Del(int index) override {
        auto& tracks = attackData[currentState];
        if (index >= 0 && index < (int)tracks.size())
            tracks.erase(tracks.begin() + index);
    }

    // --- ユーティリティ ---

    std::vector<AnimTrack>& CurrentTracks() { return attackData[currentState]; }

    float GetAnimationLength(T state) const {
        if (!model) return 1.0f;
        // AnimationStateManagerの型に注意（Tを使用）

        auto& manager = AnimationStateManager<T>::Instance();
        const auto* config = manager.GetConfig(state);
        if (!config) return 1.0f;

        int index = model->GetAnimationIndex(config->animationName.c_str());
        if (index < 0) return 1.0f;
        return model->GetAnimations()[index].secondsLength;
    }

    bool IsHitActive(T state, float currentSeconds, HandType hand = HandType::None) const {
        auto it = attackData.find(state);
        if (it == attackData.end()) return false;
        for (auto& track : it->second) {
            if (track.type != TrackType::HitBox) continue;
            if (hand != HandType::None && track.hand != hand) continue;
            if (currentSeconds >= track.GetStartSeconds() &&
                currentSeconds <= track.GetEndSeconds())
                return true;
        }
        return false;
    }

    const AnimTrack* GetActiveHitTrack(T state, float currentSec, HandType hand) {
        if (attackData.find(state) == attackData.end()) return nullptr;

        for (const auto& track : attackData.at(state)) {
            if (currentSec >= track.start * 0.01f && currentSec <= track.end * 0.01f) {
                if (hand == HandType::None || track.hand == hand) {
                    return &track; // 当たっているデータをまるごと返す
                }
            }
        }
        return nullptr;
    }

    void RenumberTracks(std::vector<AnimTrack>& tracks)
    {
        for (int i = 0; i < (int)tracks.size(); ++i)
            tracks[i].label = u8"判定 " + std::to_string(i + 1);
    }

    // --- JSON入出力 ---

    void Save(const std::string& filepath) const {
        // 仮で作る後でりんちゃんのやつをまねて作り直す
        std::filesystem::path path(filepath);
        if (path.has_parent_path() && !std::filesystem::exists(path.parent_path())) {
            std::filesystem::create_directories(path.parent_path());
        }

        nlohmann::json root;
        for (auto& [state, tracks] : attackData) {
            nlohmann::json trackArray = nlohmann::json::array();
            for (auto& t : tracks) {
                nlohmann::json track;
                track["start"] = t.start;
                track["end"] = t.end;
                track["label"] = t.label;
                track["color"] = t.color;
                track["type"] = (int)t.type;
                track["hand"] = (int)t.hand;
                track["boneName"] = t.boneName;
                track["sphereRadius"] = t.sphereRadius;
                track["sphereOffset"] = nlohmann::json::array({ t.sphereOffset.x, t.sphereOffset.y, t.sphereOffset.z, });
                track["damageRate"] = t.damageRate;
                track["invincible"] = t.invincible;
                track["poiseRate"] = t.poiseRate;
                track["soundName"] = t.soundName;
                trackArray.push_back(track);
            }
            root[std::to_string((int)state)] = trackArray;
        }

        std::ofstream file(filepath);
        if (file.is_open()) file << root.dump(4);
    }

    void Load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return; // ファイルが無ければ何もしない

        try
        {
            nlohmann::json root;
            file >> root;

            attackData.clear();
            for (auto& [key, trackArray] : root.items()) {
                T state = (T)std::stoi(key);
                std::vector<AnimTrack> tracks;
                for (auto& t : trackArray) {
                    AnimTrack track;
                    track.start = t["start"];
                    track.end = t["end"];
                    track.label = t.value("label", "No Name");
                    track.color = t.value("color", 0xFFFFFFFF);
                    track.type = (TrackType)t.value("type", 0);
                    track.hand = (HandType)t.value("hand", 0);
                    track.boneName = t.value("boneName", "");
                    track.sphereRadius = t.value("sphereRadius", 0.5f);
                    if (t.count("sphereOffset"))
                    {
                        track.sphereOffset.x = t["sphereOffset"][0];
                        track.sphereOffset.y = t["sphereOffset"][1];
                        track.sphereOffset.z = t["sphereOffset"][2];
                    }
                    else
                    {
                        track.sphereOffset = { 0, 0, 0 };
                    }
                    track.damageRate = t.value("damageRate", 0.0f);
                    track.invincible = t.value("invincible", 0.3f);
                    track.poiseRate = t.value("poiseRate", 0.0f);
                    track.soundName = t.value("soundName", "");
                    track.soundPlayed = false;
                    tracks.push_back(track);
                }
                attackData[state] = tracks;
            }
        }
        catch (const nlohmann::json::exception& e)
        {
            // 中身が空、もしくは壊れている場合はクラッシュさせずにエラーログだけ出す
            std::cerr << "JSON Load Error (" << filepath << "): " << e.what() << std::endl;
            // ImGuiのコンソール等があればそこに出力するとなお良し
        }

    }

private:
    std::shared_ptr<Model> model;
};