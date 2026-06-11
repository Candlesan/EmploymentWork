#include "AnimationSequencerEditor.h"
#include "imgui.h"
#include <algorithm>


void AnimationSequencerEditor::DrawEditor(
	AnimationSequencer& sequencer,
	const std::string& animName,
	float              animTime,
	float              animLength)
{
	ImGui::Begin("Sequencer");

	// アニメーションデータが存在しなければ自動追加
	auto& animations = sequencer.GetAnimations();
	bool found = false;
	for (auto& a : animations)
	{
		if (a.name == animName)
		{
			found = true; break;
		}
	}

	if (!found && !animName.empty())
	{
		AnimationSequencer::AnimationData newData;
		newData.name = animName;
		animations.push_back(newData);
	}

	// 現在のアニメーションデータを取得
	AnimationSequencer::AnimationData* data = nullptr;
	for (auto& a : animations)
	{
		if (a.name == animName)
		{
			data = &a;
			break;
		}
	}

	if (!data)
	{
		ImGui::TextDisabled(u8"アニメーションデータがありません");
		ImGui::End();
		return;
	}

	// ツールバー
	DrawToolbar(*data, animTime, animLength, sequencer);
	ImGui::Separator();

	// 左右レイアウト
	float detailWidth = 350.0f;
	float timelineWidth = ImGui::GetContentRegionAvail().x - detailWidth - 8.0f;

	// 左：タイムライン
	ImGui::BeginChild("TimelineArea", ImVec2(timelineWidth, 0), false);
	// イベントトラック
	DrawEventTrack(*data, animTime, animLength);
	ImGui::Spacing();
	// レンジトラック
	DrawRangeTrack(*data, animTime, animLength);
	ImGui::Spacing();
	// 速度カーブ描画
	DrawSpeedCurveTrack(*data, animTime, animLength);
	ImGui::EndChild();

	ImGui::SameLine();

	// 右：詳細パネル
	ImGui::BeginChild("DetailPanel", ImVec2(detailWidth, 0), true);
	DrawDetailPanel(*data);
	ImGui::EndChild();

	ImGui::End();
}

// ツールバー描画
void AnimationSequencerEditor::DrawToolbar(AnimationSequencer::AnimationData& data, float animTime, float animLength, AnimationSequencer& sequencer)
{
	// イベント追加ボタン
	if (ImGui::Button(u8"+ Event"))
	{
		AnimationSequencer::Event e;
		e.name = "NewEvent";
		e.time = animTime;
		data.events.push_back(e);
		selectedIndex = (int)data.events.size() - 1;
		// 新規追加したら名前バッファも更新
		strncpy_s(nameBuf, e.name.c_str(), sizeof(nameBuf));
	}
	ImGui::SameLine();

	// レンジ追加ボタン
	if (ImGui::Button(u8"+ Range"))
	{
		const float rangeLen = 0.2f;
		AnimationSequencer::Range r;
		r.name = "HitBox";
		r.start = animTime;
		r.end = std::min(animLength, animTime + rangeLen);
		data.ranges.push_back(r);
		selectedType = SelectionType::Range;
		selectedIndex = (int)data.ranges.size() - 1;
		strncpy_s(nameBuf, r.name.c_str(), sizeof(nameBuf));
	}
	ImGui::SameLine();

	// スピードキー追加ボタン
	if (ImGui::Button(u8"+ Speed Key"))
	{
		// すでにキーがある場合は追加しない
		if (data.speedKeys.empty())
		{
			data.speedKeys.push_back({ 0.0f, 1.0f });
			data.speedKeys.push_back({ animLength * 0.5f, 1.0f });
			data.speedKeys.push_back({ animLength, 1.0f });
			selectedType = SelectionType::SpeedKey;
			selectedIndex = 1;
		}
		else
		{
			data.speedKeys.push_back({ animTime, 1.0f });
			std::sort(data.speedKeys.begin(), data.speedKeys.end(),
				[](const AnimationSequencer::SpeedKey& a,
					const AnimationSequencer::SpeedKey& b)
				{ return a.time < b.time; });
		}
	}
	ImGui::SameLine();

	// 保存ボタン
	if (ImGui::Button(u8"Save"))
	{
		if (!jsonPath.empty())
		{
			sequencer.Save(jsonPath);
		}
	}
}
// イベントトラック描画
void AnimationSequencerEditor::DrawEventTrack(AnimationSequencer::AnimationData& data, float animTime, float animLength)
{
	if (animLength <= 0.0f) animLength = 1.0f;

	const float trackHeight = 32.0f;
	const float labelWidth = 60.0f;
	const float markerWidth = 10.0f;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2      cursor = ImGui::GetCursorScreenPos();
	float       width = ImGui::GetContentRegionAvail().x - labelWidth;
	if (width <= 0.0f)
	{
		width = 1.0f;
	}

	// 時刻をX座標に変換
	auto timeToX = [&](float t) -> float
		{
			return cursor.x + labelWidth + (t / animLength) * width;
		};

	// ラベル
	ImGui::SetCursorScreenPos(cursor);
	ImGui::TextDisabled("Events");
	cursor = ImGui::GetCursorScreenPos();

	ImVec2 trackMin = { cursor.x + labelWidth, cursor.y };
	ImVec2 trackMax = { trackMin.x + width, trackMin.y + trackHeight };

	// トラック背景
	dl->AddRectFilled(trackMin, trackMax, IM_COL32(40, 40, 40, 150), 4.0f);
	dl->AddRect(trackMin, trackMax, IM_COL32(80, 80, 80, 150), 4.0f);

	// プレイヘッド
	float phX = timeToX(animTime);
	dl->AddLine({ phX, trackMin.y }, { phX, trackMax.y }, IM_COL32(29, 158, 117, 200), 1.5f);

	// Eventマーカー
	for (int i = 0; i < (int)data.events.size(); i++)
	{
		auto& e = data.events[i];
		float mx = timeToX(e.time);
		bool  isSel = (selectedIndex == i);
		ImU32 color = isSel
			? IM_COL32(255, 140, 100, 255)
			: IM_COL32(226, 75, 74, 210);

		// 縦線
		dl->AddLine(
			{ mx, trackMin.y + 2 },
			{ mx, trackMax.y - 2 },
			color, 2.0f);

		// 上部ひし形
		dl->AddCircleFilled({ mx, trackMin.y + 7 }, 5.0f, color);

		// 名前テキスト
		dl->AddText(
			{ mx + 6, trackMin.y + 2 },
			IM_COL32(255, 255, 255, 200),
			e.name.c_str());

		// ドラッグ・クリック用のInvisibleButton
		ImGui::PushID(i);
		ImGui::SetCursorScreenPos({ mx - markerWidth * 0.5f, trackMin.y });
		ImGui::InvisibleButton("##ev", { markerWidth, trackHeight });

		// クリックで選択
		if (ImGui::IsItemClicked())
		{
			selectedType = SelectionType::Event;
			selectedIndex = i;
			strncpy_s(nameBuf, e.name.c_str(), sizeof(nameBuf));
		}

		// ドラッグで時刻移動
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
		{
			float delta = ImGui::GetIO().MouseDelta.x;
			e.time = std::clamp(
				e.time + delta / width * animLength,
				0.0f, animLength);
			selectedType = SelectionType::Event;
			selectedIndex = i;
		}
		ImGui::PopID();
	}
	// カーソルを次の行へ
	ImGui::SetCursorScreenPos({ cursor.x, trackMin.y + trackHeight + 4 });
	ImGui::Dummy({ width + labelWidth, 0 });
}
// 範囲トラック描画
void AnimationSequencerEditor::DrawRangeTrack(AnimationSequencer::AnimationData& data, float animTime, float animLength)
{
	if (animLength <= 0.0f) animLength = 1.0f;

	const float trackHeight = 28.0f;
	const float handleWidth = 6.0f;
	const float labelWidth = 60.0f;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2      cursor = ImGui::GetCursorScreenPos();
	float       width = ImGui::GetContentRegionAvail().x - labelWidth;
	if (width <= 0.0f)
	{
		width = 1.0f;
	}

	// 時刻をX座標に変換
	auto timeToX = [&](float t) -> float
		{
			return cursor.x + labelWidth + (t / animLength) * width;
		};

	ImGui::TextDisabled("Ranges");

	for (int i = 0; i < (int)data.ranges.size(); i++)
	{
		auto& r = data.ranges[i];
		bool  sel = (selectedType == SelectionType::Range && selectedIndex == i);

		ImGui::PushID(100 + i);

		ImVec2 rowPos = ImGui::GetCursorScreenPos();
		ImVec2 rTrackMin = { rowPos.x + labelWidth, rowPos.y };
		ImVec2 rTrackMax = { rTrackMin.x + width, rTrackMin.y + trackHeight };

		// ラベル
		ImGui::Text(r.name.c_str());
		ImGui::SameLine(labelWidth);

		// トラック背景
		dl->AddRectFilled(rTrackMin, rTrackMax, IM_COL32(40, 40, 40, 80), 4.0f);

		float sx = timeToX(r.start);
		float ex = timeToX(r.end);

		ImU32 fillColor = sel ? IM_COL32(120, 100, 220, 150) : IM_COL32(83, 74, 183, 120);
		ImU32 borderColor = sel ? IM_COL32(180, 160, 255, 255) : IM_COL32(83, 74, 183, 255);

		// Range本体
		dl->AddRectFilled({ sx, rTrackMin.y + 4 }, { ex, rTrackMax.y - 4 }, fillColor, 4.0f);
		dl->AddRect({ sx, rTrackMin.y + 4 }, { ex, rTrackMax.y - 4 }, borderColor, 4.0f);

		// 左端ハンドル描画
		dl->AddRectFilled({ sx, rTrackMin.y + 4 }, { sx + handleWidth, rTrackMax.y - 4 }, IM_COL32(200, 180, 255, 200));
		// 右端ハンドル描画
		dl->AddRectFilled({ ex - handleWidth, rTrackMin.y + 4 }, { ex, rTrackMax.y - 4 }, IM_COL32(200, 180, 255, 200));

		// 中央ドラッグ（Range全体移動）
		float centerW = std::max(0.0f, (ex - sx) - handleWidth * 2);
		if (centerW > 0.0f)
		{
			ImGui::SetCursorScreenPos({ sx + handleWidth, rTrackMin.y });
			ImGui::InvisibleButton("##center", { centerW, trackHeight });

			// クリックで選択
			if (ImGui::IsItemClicked())
			{
				selectedType = SelectionType::Range;
				selectedIndex = i;
				strncpy_s(nameBuf, r.name.c_str(), sizeof(nameBuf));
			}

			if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
			{
				float dt = ImGui::GetIO().MouseDelta.x / width * animLength;
				float len = r.end - r.start;
				r.start = std::clamp(r.start + dt, 0.0f, animLength - len);
				r.end = r.start + len;
				selectedType = SelectionType::Range;
				selectedIndex = i;
			}
		}

		// 左端ハンドル（start移動）
		ImGui::SetCursorScreenPos({ sx, rTrackMin.y });
		ImGui::InvisibleButton("##start", { handleWidth, trackHeight });

		if (ImGui::IsItemClicked())
		{
			selectedType = SelectionType::Range;
			selectedIndex = i;
			strncpy_s(nameBuf, r.name.c_str(), sizeof(nameBuf));
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
		{
			float dt = ImGui::GetIO().MouseDelta.x / width * animLength;
			r.start = std::clamp(r.start + dt, 0.0f, std::max(0.0f, r.end - 0.01f));
			selectedType = SelectionType::Range;
			selectedIndex = i;
		}

		// 右端ハンドル（end移動）
		ImGui::SetCursorScreenPos({ ex - handleWidth, rTrackMin.y });
		ImGui::InvisibleButton("##end", { handleWidth, trackHeight });

		if (ImGui::IsItemClicked())
		{
			selectedType = SelectionType::Range;
			selectedIndex = i;
			strncpy_s(nameBuf, r.name.c_str(), sizeof(nameBuf));
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
		{
			float dt = ImGui::GetIO().MouseDelta.x / width * animLength;
			r.end = std::clamp(r.end + dt,
				std::min(animLength, r.start + 0.01f), animLength);
			selectedType = SelectionType::Range;
			selectedIndex = i;
		}

		ImGui::SetCursorScreenPos({ rowPos.x, rowPos.y + trackHeight + 4 });
		ImGui::Dummy({ width + labelWidth, 0 });
		ImGui::PopID();
	}
}
// 速度カーブトラック描画
void AnimationSequencerEditor::DrawSpeedCurveTrack(AnimationSequencer::AnimationData& data, float animTime, float animLength)
{
	if (animLength <= 0.0f) animLength = 1.0f;

	const float curveHeight = 80.0f;
	const float labelWidth = 60.0f;
	const float minSpeed = 0.0f;
	const float maxSpeed = 3.0f;
	const float pointRadius = 5.0f;
	const float hitRadius = 8.0f;

	ImDrawList* dl = ImGui::GetWindowDrawList();
	ImVec2      cursor = ImGui::GetCursorScreenPos();
	float       width = ImGui::GetContentRegionAvail().x - labelWidth;
	if (width <= 0.0f) width = 1.0f;

	// ラベル
	ImGui::SetCursorScreenPos(cursor);
	ImGui::TextDisabled("Speed Curve");
	cursor = ImGui::GetCursorScreenPos();

	ImVec2 areaMin = { cursor.x + labelWidth, cursor.y };
	ImVec2 areaMax = { areaMin.x + width, areaMin.y + curveHeight };

	// 座標変換
	auto timeToX = [&](float t) -> float
		{
			return areaMin.x + (t / animLength) * width;
		};
	auto speedToY = [&](float s) -> float
		{
			return areaMin.y + (1.0f - (s - minSpeed) / (maxSpeed - minSpeed)) * curveHeight;
		};

	// 背景
	dl->AddRectFilled(areaMin, areaMax, IM_COL32(30, 30, 30, 150), 4.0f);
	dl->AddRect(areaMin, areaMax, IM_COL32(80, 80, 80, 150), 4.0f);

	// speed=1.0 の基準ライン
	float baseY = speedToY(1.0f);
	dl->AddLine({ areaMin.x, baseY }, { areaMax.x, baseY }, IM_COL32(255, 255, 255, 40), 1.0f);

	// プレイヘッド
	float phX = timeToX(animTime);
	dl->AddLine({ phX, areaMin.y }, { phX, areaMax.y }, IM_COL32(29, 158, 117, 180), 1.5f);

	// カーブ描画（キーポイント間を線で結ぶ）
	auto& keys = data.speedKeys;
	if (keys.size() >= 2)
	{
		for (int i = 0; i + 1 < (int)keys.size(); i++)
		{
			ImVec2 a = { timeToX(keys[i].time),     speedToY(keys[i].speed) };
			ImVec2 b = { timeToX(keys[i + 1].time),   speedToY(keys[i + 1].speed) };
			dl->AddLine(a, b, IM_COL32(255, 200, 50, 200), 2.0f);
		}
	}
	else if (keys.size() == 1)
	{
		// キーが1つの場合は水平線
		float py = speedToY(keys[0].speed);
		dl->AddLine({ areaMin.x, py }, { areaMax.x, py }, IM_COL32(255, 200, 50, 200), 2.0f);
	}

	// キーポイント描画・ドラッグ
	for (int i = 0; i < (int)keys.size(); i++)
	{
		float px = timeToX(keys[i].time);
		float py = speedToY(keys[i].speed);
		bool  sel = (selectedType == SelectionType::SpeedKey && selectedIndex == i);
		ImU32 col = sel ? IM_COL32(255, 220, 80, 255) : IM_COL32(255, 200, 50, 200);

		// ヒット判定用InvisibleButton
		ImGui::PushID(200 + i);
		ImGui::SetCursorScreenPos({ px - hitRadius, py - hitRadius });
		ImGui::InvisibleButton("##sk", { hitRadius * 2, hitRadius * 2 });

		if (ImGui::IsItemClicked())
		{
			selectedType = SelectionType::SpeedKey;
			selectedIndex = i;
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0))
		{
			ImVec2 delta = ImGui::GetIO().MouseDelta;

			// 時刻移動（端のキーは移動させない）
			if (i != 0 && i != (int)keys.size() - 1)
			{
				float newTime = keys[i].time + delta.x / width * animLength;
				float minTime = keys[i - 1].time + 0.001f;
				float maxTime = keys[i + 1].time - 0.001f;
				keys[i].time = std::clamp(newTime, minTime, maxTime);
			}

			// 速度移動
			float newSpeed = keys[i].speed - delta.y / curveHeight * (maxSpeed - minSpeed);
			keys[i].speed = std::clamp(newSpeed, minSpeed, maxSpeed);

			selectedType = SelectionType::SpeedKey;
			selectedIndex = i;
		}

		// 点を描画
		dl->AddCircleFilled({ px, py }, pointRadius, col);
		dl->AddCircle({ px, py }, pointRadius, IM_COL32(255, 255, 255, 200));

		// 速度の数値表示
		char buf[16];
		snprintf(buf, sizeof(buf), "x%.2f", keys[i].speed);
		dl->AddText({ px + pointRadius + 2, py - 8 }, IM_COL32(220, 220, 220, 220), buf);

		ImGui::PopID();
	}

	// カーソルを次の行へ
	ImGui::SetCursorScreenPos({ cursor.x, areaMax.y + 4 });
	ImGui::Dummy({ width + labelWidth, 0 });
}

// 詳細パネル描画
void AnimationSequencerEditor::DrawDetailPanel(AnimationSequencer::AnimationData& data)
{
	ImGui::TextDisabled(u8"詳細");
	ImGui::Separator();

	// イベント
	if (selectedType == SelectionType::Event &&
		selectedIndex >= 0 &&
		selectedIndex < (int)data.events.size())
	{
		auto& e = data.events[selectedIndex];

		ImGui::Text("Event");
		ImGui::Separator();
		ImGui::Spacing();

		// 名前編集
		if (ImGui::InputText("Name##Event", nameBuf, sizeof(nameBuf)))
		{
			e.name = nameBuf;
		}


		// 時刻表示
		ImGui::DragFloat("Time##Event", &e.time, 0.001f, 0.0f, 100.0f, "%.3f");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 削除
		if (ImGui::Button(u8"削除", { -1, 0 }))
		{
			data.events.erase(data.events.begin() + selectedIndex);
			selectedIndex = -1;
			memset(nameBuf, 0, sizeof(nameBuf));
		}

	}
	// レンジ
	else if (selectedType == SelectionType::Range &&
		selectedIndex >= 0 &&
		selectedIndex < (int)data.ranges.size())
	{
		auto& r = data.ranges[selectedIndex];

		ImGui::TextColored(ImVec4(0.5f, 0.4f, 0.9f, 1.0f), "Range (HitBox) Settings");
		ImGui::Spacing();

		if (ImGui::InputText("Label Name", nameBuf, sizeof(nameBuf))) r.name = nameBuf;
		ImGui::DragFloat("Start Time", &r.start, 0.001f, r.start, 100.0f);
		ImGui::DragFloat("End Time", &r.end, 0.001f, 0.0f, r.end);

		ImGui::Checkbox(u8"Is HitBox(当たり判定として使う)", &r.isHitBox);

		ImGui::Separator();

		if (r.isHitBox)
		{
			const char* handItems[] = { "None", "RightHand", "LeftHand", "Both", "Body", "Magic" };
			int handIndex = (int)r.hand;
			if (ImGui::Combo(u8"Hand(判定部位)", &handIndex, handItems, IM_ARRAYSIZE(handItems)));
			{
				r.hand = (HandType)handIndex;
			}

			// Bodyの時はボーン追従の設定を出す
			if (r.hand == HandType::Body) {
				char boneBuf[128];
				strncpy_s(boneBuf, r.boneName.c_str(), sizeof(boneBuf));
				if (ImGui::InputText(u8"Bone Name", boneBuf, sizeof(boneBuf))) r.boneName = boneBuf;
				ImGui::DragFloat(u8"Sphere Radius", &r.sphereRadius, 0.01f, 0.1f, 5.0f);
				ImGui::DragFloat3(u8"Sphere Offset", &r.sphereOffset.x, 0.01f);
			}

			ImGui::DragFloat(u8"Damage Rate", &r.damageRate, 0.01f, 0.1f, 10.0f);
			ImGui::DragFloat(u8"Poise Rate (削り)", &r.poiseRate, 0.01f);
			ImGui::DragFloat(u8"Invincible (無敵)", &r.invincible, 0.01f);
		}
		else
		{
			ImGui::TextDisabled(u8"※この範囲は汎用フラグとして通知されます。");
		}

		ImGui::Spacing();
		ImGui::Separator();
		if (ImGui::Button(u8"削除 (Delete)", { -1, 0 }))
		{
			data.ranges.erase(data.ranges.begin() + selectedIndex);
			selectedType = SelectionType::None;
			selectedIndex = -1;
			memset(nameBuf, 0, sizeof(nameBuf));
		}
	}
	// スピードキー
	else if (selectedType == SelectionType::SpeedKey &&
		selectedIndex >= 0 &&
		selectedIndex < (int)data.speedKeys.size())
	{
		auto& sk = data.speedKeys[selectedIndex];

		ImGui::Text("Speed Key");
		ImGui::Separator();
		ImGui::Spacing();

		// 端のキーは時刻編集不可
		if (selectedIndex == 0 || selectedIndex == (int)data.speedKeys.size() - 1)
		{
			ImGui::TextDisabled("Time: %.3f", sk.time);
		}
		else
		{
			float minTime = data.speedKeys[selectedIndex - 1].time + 0.001f;
			float maxTime = data.speedKeys[selectedIndex + 1].time - 0.001f;
			ImGui::DragFloat("Time##sk", &sk.time, 0.001f, minTime, maxTime, "%.3f");
		}

		ImGui::DragFloat("Speed##sk", &sk.speed, 0.01f, 0.0f, 3.0f, "x%.2f");

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button(u8"削除", { -1, 0 }))
		{
			data.speedKeys.erase(data.speedKeys.begin() + selectedIndex);
			selectedType = SelectionType::None;
			selectedIndex = -1;
		}
	}
	// 未選択
	else
	{
		ImGui::TextDisabled(u8"未選択");
	}
}
