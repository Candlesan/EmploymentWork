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
	None,					// 条件なし
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
	
	void Save(const std::string& path); // Jsonファイルに保存する
	void Load(const std::string& path); // Jsonファイルを読み込む

	int NextId() { return nextId++; }
private:
	static int nextId;
};