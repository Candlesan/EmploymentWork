#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <DirectXMath.h>
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"


enum class EventType {
    Sound,
    Effect,
    Particle, // 作ったパーティクルを出す
};

enum class HandType {
    None,
    RightHand,
    LeftHand,
    Both, // 両手攻撃
    Body, // 体当たりなど
    Magic,
};

class AnimationSequencer
{
public:
    AnimationSequencer(const std::string& jsonPath);
    AnimationSequencer() = default;
    ~AnimationSequencer() = default;

    // 更新
    void Update(const std::string& animName, float animTime);

    // 結果取得
    bool GetEvent(const std::string& eventName) const;
    bool GetRange(const std::string& flagName) const;
    bool GetRangeStart(const std::string& name) const;  // 入った瞬間だけtrue
    bool GetRangeEnd(const std::string& name) const;    // 出た瞬間だけtrue
    float GetSpeed() const;

    // 構造体
    struct Event
    {
        std::string name;        // イベント名
        float       time = 0.0f; // 開始時刻

        // 単発のイベント用
        EventType   type = EventType::Sound;
        std::string soundName = "";
        std::string effectName = "";
        bool        played = false; // 再生済みフラグ
    };

    struct Range
    {
        std::string name;         // フラグ名
        float       start = 0.0f; // 開始時刻
        float       end = 0.0f;   // 終了時刻

        // 攻撃の当たり判定用
        HandType hand = HandType::None;
        std::string boneName = "";   // 追従するボーン名
        float sphereRadius = 0.5f;   // 球の半径
        DirectX::XMFLOAT3 sphereOffset = { 0, 0, 0 }; // ボーンからのオフセット
        float damageRate = 1.0f;     // ダメージ倍率
        float poiseRate = 1.0f;      // 削り値
        float invincible = 0.3f;     // 無敵時間

        bool isHitBox = true;
    };

    struct SpeedKey
    {
        float time = 0.0f;  // キーフレームの時刻
        float speed = 1.0f; // その時刻での再生速度倍率
    };

    
    struct AnimationData
    {
        std::string           name;      // アニメーション名
        std::vector<Event>    events;    // イベント一覧
        std::vector<Range>    ranges;    // レンジ一覧
        std::vector<SpeedKey> speedKeys; // スピードキー一覧
    };

    // ゲッター
    std::vector<AnimationData>& GetAnimations() { return animations; }
    const std::vector<AnimationData>& GetAnimations() const { return animations; }

    // 保存/読み込み
    void Save(const std::string& jsonPath);
    void Load(const std::string& jsonPath);

    // 再読み込み
    void Reload() { Load(jsonPath); }

private:
    std::vector<AnimationData> animations;

    // 現在のアニメーション名
    std::string currentAnimName;
    // 前フレームのアニメーション時刻
    float prevAnimTime = 0.0f;
    float currentSpeed = 1.0f;

    std::unordered_map<std::string, bool> prevActiveFlags; // 前の状態
    std::unordered_map<std::string, bool> rangeStarted;  // 前フレームOFF→今フレームON
    std::unordered_map<std::string, bool> rangeEnded;    // 前フレームON→今フレームOFF

    // 使用済みイベント
    std::unordered_map<std::string, bool> firedEvents;
    // 現在有効なフラグ
    std::unordered_map<std::string, bool> activeFlags;

    // 現在のアニメーションデータを検索
    AnimationData* FindAnimationData(const std::string& animName);

    // 自動リロード用
    std::string jsonPath;
    std::filesystem::file_time_type lastWriteTime;
    void CheckAndReload();
};