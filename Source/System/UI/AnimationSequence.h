#pragma once
#include "ImSequencer.h"
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"
#include <unordered_map>
#include <string>

enum class TrackType {
    HitBox,
    Effect,
    Sound,
};

struct AnimTrack {
    int start;        // 秒×100の整数値 (例: 0.10秒 → 10)
    int end;          // 秒×100の整数値 (例: 0.25秒 → 25)
    std::string label;
    unsigned int color;
    TrackType type;
    std::string effectName;
    std::string soundName;

    // 実際の秒数に変換するヘルパー
    float GetStartSeconds() const { return start / 100.0f; }
    float GetEndSeconds()   const { return end / 100.0f; }
};

class AnimationSequence : public ImSequencer::SequenceInterface
{
public:
    // キーをPlayerAnimationStateに変更
    std::unordered_map<PlayerAnimationState, std::vector<AnimTrack>> attackData;
    PlayerAnimationState currentState = PlayerAnimationState::Idle; 

    void SetModel(std::shared_ptr<Model> model) { this->model = model; }

    // stateに対応するアニメの総秒数をModelから直接引く
    float GetAnimationLength(PlayerAnimationState state) const
    {
        if (!model) return 1.0f;
        auto& manager = AnimationStateManager<PlayerAnimationState>::Instance();
        const AnimationConfig* config = manager.GetConfig(state);
        if (!config) return 1.0f;

        int index = model->GetAnimationIndex(config->animationName.c_str());
        if (index < 0) return 1.0f;
        return model->GetAnimations()[index].secondsLength;
    }

    std::vector<AnimTrack>& CurrentTracks() { return attackData[currentState]; }

    // シーケンサーの総フレーム数 = アニメの秒数×100
    // 外から設定できるようにしておく
    float currentAnimSeconds = 1.0f;

    int GetFrameMin() const override { return 0; }
    int GetFrameMax() const override { return (int)(currentAnimSeconds * 144); }

    int GetItemCount() const override {
        auto it = attackData.find(currentState);
        if (it == attackData.end()) return 0;
        return (int)it->second.size();
    }

    void Get(int index, int** start, int** end, int* type, unsigned int* color) override {
        auto& t = attackData[currentState][index];
        if (start) *start = &t.start;
        if (end)   *end = &t.end;
        if (color) *color = t.color;
        if (type)  *type = (int)t.type;
    }

    const char* GetItemLabel(int index) const override {
        return attackData.at(currentState)[index].label.c_str();
    }

    // 指定秒数がHitBox範囲内かチェック
    bool IsHitActive(PlayerAnimationState state, float currentSeconds) const {
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

private:
    std::shared_ptr<Model> model;
};