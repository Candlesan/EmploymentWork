#pragma once
#include "GamePlay/Object/Character/Animation/AnimationStateManager.h"
#include "System/Resource/Model/Model.h"
#include "ImSequencer.h"
#include "json.hpp"
#include <unordered_map>
#include <fstream>
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
    float GetAnimationLength(PlayerAnimationState state) const;

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

    void Get(int index, int** start, int** end, int* type, unsigned int* color) override;

    const char* GetItemLabel(int index) const override {
        return attackData.at(currentState)[index].label.c_str();
    }

    void Del(int index) override;

    // 指定秒数がHitBox範囲内かチェック
    bool IsHitActive(PlayerAnimationState state, float currentSeconds) const;

    // JSON保存
    void Save(const std::string& filepath) const;

    // JSON読み込み
    void Load(const std::string& filepath);
private:
    std::shared_ptr<Model> model;
};