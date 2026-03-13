#pragma once
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"
#include "System/Resource/Model/Model.h"
#include "ImSequencer.h"
#include "json.hpp"
#include <unordered_map>
#include <fstream>
#include <string>
#include <vector>

enum class TrackType {
    HitBox,
    Effect,
    Sound,
};

struct AnimTrack {
    int start;
    int end;
    std::string label;
    unsigned int color;
    TrackType type;
    std::string effectName;
    std::string soundName;

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

    bool IsHitActive(T state, float currentSeconds) const {
        auto it = attackData.find(state);
        if (it == attackData.end()) return false;
        for (auto& track : it->second) {
            if (track.type != TrackType::HitBox) continue;
            if (currentSeconds >= track.GetStartSeconds() &&
                currentSeconds <= track.GetEndSeconds())
                return true;
        }
        return false;
    }

    // --- JSON入出力 ---

    void Save(const std::string& filepath) const {
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
                trackArray.push_back(track);
            }
            root[std::to_string((int)state)] = trackArray;
        }
        std::ofstream file(filepath);
        if (file.is_open()) file << root.dump(4);
    }

    void Load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;
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
                tracks.push_back(track);
            }
            attackData[state] = tracks;
        }
    }

private:
    std::shared_ptr<Model> model;
};