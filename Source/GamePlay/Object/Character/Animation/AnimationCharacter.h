#pragma once
#include "GamePlay/Object/Character/Character.h"
#include "System/UI/AnimationNodeEditor/AnimationTransitionGraph.h"
#include "GamePlay/Object/Character/Animation/Animator.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>


// アニメーション機能を持つキャラクター基底クラス
class AnimationCharacter : public Character
{
public:
	AnimationCharacter() = default;
	virtual ~AnimationCharacter() = default;

	// ステート変更（新しいステート名を与える）
	void ChangeAnimationState(const std::string& newState, bool ignoreOverlay = false);

	// 上半身のアニメーションを開始する（オーバーレイ再生）
	void StartOverlayAnimation(const std::string& newState);

	// 毎フレームのアニメーション更新
	void UpdateAnimation(float elapsedTime);

	// アタッチメント
	void UpdateWeaponAttachment(const std::string& attachBoneName);

	// 現在のアニメーションが終了したかどうか
	bool IsAnimationFinished() const;

	// 再生時間が指定範囲内かを判定する（遷移条件に使う）
	bool IsAnimationInTimeRange(float startSeconds, float endSeconds) const;

	// 再生時間が指定範囲外かを判定する（遷移条件に使う）
	bool IsAnimationOutTimeRange(float startSeconds, float endSeconds) const;

	// 再生時間が指定位置より後かを判定する（遷移条件に使う）
	bool IsAnimationOutTimeRange(float StartTransition) const;

	// 徐々にアニメーション速度を落とす補助関数
	void AnimationLerp(float StartSlow, float endSlow, float SlowSpeed);

	// 指定ノードを起点に、その子が上半身ノードかを判定するヘルパー
	bool IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName);

	// 現在のステート名を取得
	std::string GetCurrentState() const { return currentState; }

	// 再生速度調整用セッター（速度ブースト系）
	void SetSpeedUp(float s) { speedUp = s; }
	void SetBaseSpeed(float s) { baseSpeed = s; }

	// 再生中の秒数を取得
	float GetCurrentAnimationSeconds() const { return animationSeconds; }
	float GetCurrentAnimationLength() const;

	// JSONからアニメーション設定を読み込む
	void LoadAnimationData(const std::string& jsonPath);

	// 指定ステートの設定を取得（読み取り専用）
	const AnimationConfig* GetAnimationConfig(const std::string& stateName) const;

protected:
	// サブクラス向けのステート変化コールバック（必要ならオーバーライド）
	virtual void OnStateChanged(const std::string& oldState, const std::string& newState) {}

	// モデルアクセス：派生クラスで実装すること（共有ポインタを返す）
	virtual std::shared_ptr<Model> GetModel() = 0;
	virtual const std::shared_ptr<Model> GetModel() const = 0;


	// === アニメーション管理用のメンバ群 ===

	// 現在・前回・オーバーレイステート名
	std::string currentState = "";
	std::string previousState = "";
	std::string currentOverlayState = "";

	// ノードポーズ（現在／直前）
	std::vector<Model::NodePose> nodePoses;
	std::vector<Model::NodePose> oldNodePoses;

	// ルートおよび上半身ノード名のデフォルト
	std::string rootMotionNodeName = "root";      // ルートモーションのノード名
	std::string upperBodyNodeName = "spine_01";  // 上半身の起点ノード名

	// ステート名 -> 設定テーブル
	std::unordered_map<std::string, AnimationConfig> stateConfigs;

	// 再生インデックス／時間管理
	int animationIndex = -1;
	float animationSeconds = 0.0f;
	float oldAnimationSeconds = 0.0f;

	// ブレンド制御
	float animationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = 0.2f;
	bool isBlending = true;
	bool justChangedAnimation = true;
	bool animationLoop = false;

	// ルートモーション関連
	bool useRootMotion = false;
	bool useRootMotionEx = false;
	bool bakeTranslationY = true;

	// ヒットストップ（再生停止）関連
	bool hitStop = false;
	float hitStopLastSeconds = 0.0f;
	float hitStopSecondsLength = 0.3f;
	float cameraShakeRange = 0.03f;

	// 再生速度制御（基本速度と追加速度）
	float baseSpeed = 1.0f;
	float speedUp = 0.0f;

	// 上半身アニメーション関連（オーバーレイ）
	int overlayAnimationIndex = -1;       // 上半身アニメのインデックス
	float overlayAnimationSeconds = 0.0f; // 上半身アニメの再生時間
	bool overlayAnimationLoop = false;
	bool isOverlayPlaying = false;        // 上半身アニメ再生中かどうか

	// スムーズ移動長さ（補助）
	float smoothMoveLength = 0.0f;


	// ワールド変換（モデルのtransform保管用）
	DirectX::XMFLOAT4X4 worldTransform = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	// アニメータ（内部実装）
	std::unique_ptr<Animator> animator;

	// 武器のアタッチメメント関係
	struct Weapon
	{
		std::shared_ptr<Model> model;
		DirectX::XMFLOAT3 position = { 0,0,0 };
		DirectX::XMFLOAT3 angle = { 0,0,0 };
		DirectX::XMFLOAT3 scale = { 1,1,1 };
		DirectX::XMFLOAT4X4 transform = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
		DirectX::XMFLOAT3 weaponHitOffset = { 0, 0, 0 };
		DirectX::XMFLOAT3 weaponAngleOffset = { 0, 0, 0 };
		float weaponRadius = 0.5f;
		float weaponHeight = 1.0f;
		bool isVisible = false;
	};
	Weapon weapon;

};