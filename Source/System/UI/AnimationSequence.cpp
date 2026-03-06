#include "AnimationSequence.h"


// stateに対応するアニメの総秒数をModelから直接引く
float AnimationSequence::GetAnimationLength(PlayerAnimationState state) const
{
    if (!model) return 1.0f;
    auto& manager = AnimationStateManager<PlayerAnimationState>::Instance();
    const AnimationConfig* config = manager.GetConfig(state);
    if (!config) return 1.0f;

    int index = model->GetAnimationIndex(config->animationName.c_str());
    if (index < 0) return 1.0f;
    return model->GetAnimations()[index].secondsLength;
}

void AnimationSequence::Get(int index, int** start, int** end, int* type, unsigned int* color)
{
    auto& t = attackData[currentState][index];
    if (start) *start = &t.start;
    if (end)   *end = &t.end;
    if (color) *color = t.color;
    if (type)  *type = (int)t.type;
}

void AnimationSequence::Del(int index)
{
    auto& tracks = attackData[currentState];
    if (index >= 0 && index < (int)tracks.size())
        tracks.erase(tracks.begin() + index);
}

// 指定秒数がHitBox範囲内かチェック
bool AnimationSequence::IsHitActive(PlayerAnimationState state, float currentSeconds) const
{
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

// JSON保存
void AnimationSequence::Save(const std::string& filepath) const
{
    nlohmann::json root;

    for (auto& [state, tracks] : attackData)
    {
        nlohmann::json trackArray;
        for (auto& t : tracks)
        {
            nlohmann::json track;
            track["start"] = t.start;
            track["end"] = t.end;
            track["label"] = t.label;
            track["color"] = t.color;
            track["type"] = t.type;
            trackArray.push_back(track);
        }
        // enumをintに変換してキーにする
        root[std::to_string((int)state)] = trackArray;
    }

    std::ofstream file(filepath);
    file << root.dump(4); // 4スペースで整形
}

// JSON読み込み
void AnimationSequence::Load(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    nlohmann::json root;
    file >> root;

    attackData.clear();
    for (auto& [key, trackArray] : root.items())
    {
        PlayerAnimationState state = (PlayerAnimationState)std::stoi(key);
        std::vector<AnimTrack> tracks;
        for (auto& t : trackArray)
        {
            AnimTrack track;
            track.start = t["start"];
            track.end = t["end"];
            track.label = t["label"];
            track.color = t["color"];
            track.type = (TrackType)(int)t["type"];
            tracks.push_back(track);
        }
        attackData[state] = tracks;
    }
}
