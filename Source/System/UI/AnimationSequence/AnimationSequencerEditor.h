#pragma once
#include "System/UI/AnimationSequence/AnimationSequence.h"
#include <string>


class AnimationSequencerEditor
{
public:
	AnimationSequencerEditor() = default;
	~AnimationSequencerEditor() = default;

	// エディタ描画
	void DrawEditor(
		AnimationSequencer& sequencer,
		const std::string& animName,
		float               animTime,
		float               animLength
	);

	// JSONパス設定
	void SetJsonPath(const std::string& path) { jsonPath = path; }

	void SetWindowName(const std::string& name) { windowName = name; }
private:
	std::string jsonPath;
	std::string windowName = "Sequencer";

	// 選択タイプ
	enum class SelectionType { None, Event, Range, SpeedKey };
	SelectionType selectedType = SelectionType::None;
	// 選択中のインデックス
	int selectedIndex = -1;

	// 名前バッファ
	char nameBuf[128] = {};


	// ツールバー描画
	void DrawToolbar(AnimationSequencer::AnimationData& data, float animTime, float animLength, AnimationSequencer& sequencer);

	// イベントトラック描画
	void DrawEventTrack(AnimationSequencer::AnimationData& data, float animTime, float animLength);

	// 範囲トラック描画
	void DrawRangeTrack(AnimationSequencer::AnimationData& data, float animTime, float animLength);

	// 速度カーブトラック描画
	void DrawSpeedCurveTrack(AnimationSequencer::AnimationData& data, float animTime, float animLength);

	// 詳細パネル描画
	void DrawDetailPanel(AnimationSequencer::AnimationData& data);


};