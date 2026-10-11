#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <imgui_node_editor.h>
#include "json.hpp"

#include "GamePlay/Object/Character/Enemy/BehaviorTree/Base/BehaviorTree.h"
#include "GamePlay/Object/Character/Animation/AnimationConfig.h"

namespace ed = ax::NodeEditor;

// 中間ノードの判断条件の種類
enum class BTConditionType
{
	None,					// 条件なし
	Distance,				// プレイヤーとの距離
	BossHpRate,				// ボスのHPの割合
	timer,					// タイマー
	AttackCoolDown,			// 攻撃のクールダウン
	EnemyCurrentMagic,		// 敵の現在の魔力残量
	EnemyCurrentHeat,		// 敵の現在のヒート値
	EnemyCurrentDefense,	// 敵の現在の防御魔法の耐久値
	PlayerCurrentMagic,		// プレイヤーの現在の魔力残量
	PlayerCurrentHeat,		// プレイヤーの現在のヒート値
	PlayerCurrentDefense,	// プレイヤーの現在の防御魔法の耐久値
	IsPlayerDefending,		// プレイヤーが防御魔法を展開中か (1.0=Yes, 0.0=No)
	IncomingMagicCount,		// 自身に向かって飛んできている魔法の数 (迎撃判断用)
	IsBurst,				// バースト状態か
	IsBreak,				// バリアが壊れたか
	Count,					// enumの総数
};

// 比較演算子
enum class BTCompareType
{
	Greater,       // >
	Less,          // <
	GreaterEqual,  // >=
	LessEqual,     // <=
	Equal,         // ==
	NotEqual       // !=
};

// 条件構造体
struct BTCondition
{
	int conditionType = 0; // 条件の種類を格納する変数
	int compareType = 0; // 比較演算子の種類
	float judgmentValue = 0.0f; // 判定値
};

// アクションタイプ
enum class BTActionType
{
	None = 0,
	Idle,
	MoveForward,
	MoveBack,
	Right,
	Left,
	NormalMagicAttack,
	RapidFire,
	Intercept,
	Guard,
	DropGuard,
};

struct BTConditionInfo
{
	BTConditionType type;
	const char* id;		// JSONに保存する名前
	const char* label;	// エディターの表示名
};

inline const BTConditionInfo kConditionInfos[] = {
	{BTConditionType::Distance,               "Distance",               u8"プレイヤーとの距離"},
	{BTConditionType::BossHpRate,             "BossHpRate",             u8"ボスのHPの割合(%)"},
	{BTConditionType::timer,                  "timer",                  u8"タイマー(秒)"},
	{BTConditionType::AttackCoolDown,         "AttackCoolDown",         u8"攻撃のクールダウン"},
	{BTConditionType::EnemyCurrentMagic,      "EnemyCurrentMagic",      u8"敵の現在の魔力残量"},
	{BTConditionType::EnemyCurrentHeat,       "EnemyCurrentHeat",       u8"敵の現在のヒート値"},
	{BTConditionType::EnemyCurrentDefense,    "EnemyCurrentDefense",    u8"敵の現在の防御魔法の耐久値"},
	{BTConditionType::PlayerCurrentMagic,     "PlayerCurrentMagic",     u8"プレイヤーの現在の魔力残量"},
	{BTConditionType::PlayerCurrentHeat,      "PlayerCurrentHeat",      u8"プレイヤーの現在のヒート値"},
	{BTConditionType::PlayerCurrentDefense,   "PlayerCurrentDefense",   u8"プレイヤーの現在の防御魔法の耐久値"},
	{BTConditionType::IsPlayerDefending,      "IsPlayerDefending",      u8"プレイヤーが防御魔法を展開中か (1.0=Yes, 0.0=No)"},
	{BTConditionType::IncomingMagicCount,     "IncomingMagicCount",     u8"自身に向かって飛んできている魔法の数"},
	{BTConditionType::IsBurst,                "IsBurst",                u8"バースト状態か"},
	{BTConditionType::IsBreak,                "IsBreak",                u8"バリアブレイクをしたか"},
	{BTConditionType::None,                   "None",                   u8"条件なし"},
};

struct BTActionInfo
{
	BTActionType type;
	const char* id;		// JSONに保存する名前
	const char* label;	// エディターの表示名
};
inline const BTActionInfo kActionInfos[] = {
	{BTActionType::None,                 "None",                 u8"なし"},
	{BTActionType::Idle,                 "Idle",                 u8"待機"},
	{BTActionType::MoveForward,          "MoveForward",          u8"前進"},
	{BTActionType::MoveBack,             "MoveBack",             u8"後退"},
	{BTActionType::Right,                "Right",                u8"右"},
	{BTActionType::Left,                 "Left",                 u8"左"},
	{BTActionType::NormalMagicAttack,    "NormalMagicAttack",    u8"通常攻撃魔法"},
	{BTActionType::RapidFire,            "RapidFire",            u8"連射魔法"},
	{BTActionType::Intercept,            "Intercept",            u8"迎撃"},
	{BTActionType::Guard,                "Guard",                u8"防御魔法展開"},
	{BTActionType::DropGuard,            "DropGuard",            u8"防御魔法解除"},
};

// ノードデータ
struct BTNode
{
	ed::NodeId nodeId; // 識別番号
	ed::PinId pinIn; // 親から繋げる
	ed::PinId pinOut; // 子へ繋げる

	std::string name; // ノード名
	std::string parentName; // 親の名前

	BehaviorTree::SelectRule selectRule = BehaviorTree::SelectRule::Non;
	int priority = 1; // 優先順位

	AnimationConfig config; // アニメーション設定

	ImVec2 position = { 0, 0 }; // ノードの位置

	BTActionType actionType = BTActionType::None; // BTアクションの種類

	std::vector<BTCondition> conditions; // 条件情報郡

	bool interruptible = false; // 再評価用の変数
};

// 親子を繋ぐ線
struct BTLink
{
	ed::LinkId linkId;
	ed::PinId startPin; // 親ノード
	ed::PinId endPin;  // 子ノード
};

struct BehaviorTreeGraph
{
public:
	std::vector<BTNode> nodes;
	std::vector<BTLink> links;
	int justLoad = 0;

	void AddNode(const std::string& StateName, ImVec2 pos); // ノードの追加
	void AddLink(ed::PinId from, ed::PinId to); // 矢印の追加
	void RemoveNode(ed::NodeId id); // ノードの削除
	void RemoveLink(ed::LinkId id); // 矢印の削除
	void UpdateParentNames();
	
	static std::string ConditionToId(int type);
	static int ReadConditionType(const nlohmann::json& value);
	static std::string ActionToId(int type);
	static int ReadActionType(const nlohmann::json& value);

	void Save(const std::string& path); // Jsonファイルに保存する
	void Load(const std::string& path); // Jsonファイルを読み込む

	int NextId() { return nextId++; }
private:
	static int nextId;
};