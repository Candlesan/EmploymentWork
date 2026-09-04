#include "AnimationCharacter.h"
#include "json.hpp"
#include <fstream>


// ステート変更
void AnimationCharacter::ChangeAnimationState(const std::string& newState, bool ignoreOverlay)
{
	if (currentState == newState) return;

	// ステート設定を取得
	auto it = stateConfigs.find(newState); // Mapの中から指定されたアニメーションを探す
	if (it == stateConfigs.end()) return; // 見つからなかったら無視する

	const AnimationConfig& config = it->second;

	// モデルを取得
	std::shared_ptr<Model> model = GetModel();
	if (!model) return;

	// アニメーションインデックス取得
	int newAnimationIndex = model->GetAnimationIndex(config.animationName.c_str());
	if (newAnimationIndex < 0) return;

	// コールバック
	OnStateChanged(currentState, newState);

	// ステ-ト更新
	previousState = currentState;
	currentState = newState;

	// アニメーション切り替え
	if (animationIndex != newAnimationIndex)
	{
		oldNodePoses = nodePoses;
		animationIndex = newAnimationIndex;
		animationSeconds = 0.0f;
		oldAnimationSeconds = 0.0f;
		animationBlendSeconds = 0.0f;
		isBlending = true;
		justChangedAnimation = true;
	}

	//アニメーションのリソースが同じでも、ステートが変われば「ループ設定」などは更新
	animationLoop = config.loop;
	useRootMotion = config.useRootMotion;
	useRootMotionEx = config.useRootMotionEx;
	animationBlendSecondsLength = config.blendTime;

	if (animator)
	{
		animator->PlayBase(newAnimationIndex, config.loop, animationBlendSecondsLength, useRootMotion, useRootMotionEx);
	}
}

// 上半身のアニメーション
void AnimationCharacter::StartOverlayAnimation(const std::string& newState)
{
	if (currentOverlayState == newState) return;

	auto it = stateConfigs.find(newState);
	if (it == stateConfigs.end()) return;

	currentOverlayState = newState;

	const AnimationConfig& config = it->second;

	std::shared_ptr<Model> model = GetModel();
	if (!model) return;

	int newAnimationIndex = model->GetAnimationIndex(config.animationName.c_str());
	if (newAnimationIndex < 0) return;

	if (animator)
	{
		// Animator工場の上半身ラインに発注するだけ！
		animator->PlayUpper(newAnimationIndex, config.loop, config.targetBoneName, config.blendTime);
	}
}

// アニメーション更新
void AnimationCharacter::UpdateAnimation(float elapsedTime)
{
	if (animator)
	{
		// このフレームの移動量を受け取って、キャラクターの座標に足す
		DirectX::XMFLOAT3 delta = animator->Update(elapsedTime * baseSpeed, GetTransform());

		this->position.x += delta.x;
		this->position.y += delta.y;
		this->position.z += delta.z;

		this->animationSeconds = animator->GetBaseCurrentSeconds();
	}
}

// 武器のアタッチメント
void AnimationCharacter::UpdateWeaponAttachment(const std::string& attachBoneName)
{
	// 武器モデルがないなら何もしない
	if (!weapon.model) return;

	// ボーン名を入れる
	const char* AttachName = attachBoneName.c_str();

	// モデルを取得
	std::shared_ptr<Model> myModel = GetModel();
	if (!myModel) return;

	// 武器のローカル行列を計算する
	DirectX::XMMATRIX S = DirectX::XMMatrixScaling(weapon.scale.x, weapon.scale.y, weapon.scale.z);
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationRollPitchYaw(weapon.angle.x, weapon.angle.y, weapon.angle.z);
	DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(weapon.position.x, weapon.position.y, weapon.position.z);
	DirectX::XMMATRIX weaponLocal = S * R * T;

	// キャラクターモデルから右手ノードを検索する
	for (const Model::Node& node : myModel->GetNodes())
	{
		if (strcmp(node.name.c_str(), AttachName) == 0)
		{
			// 右手ノードと武器のローカル行列から武器のワールド行列を求める
			DirectX::XMMATRIX rightHandGlobal = DirectX::XMLoadFloat4x4(&node.globalTransform);
			DirectX::XMMATRIX playerWorld = DirectX::XMLoadFloat4x4(&GetTransform());
			DirectX::XMMATRIX weaponWorld = weaponLocal * rightHandGlobal * playerWorld;
			DirectX::XMStoreFloat4x4(&weapon.transform, weaponWorld);
			weapon.model->UpdateTransform(weapon.transform);
			break;
		}
	}

}

// アニメーション終了判定
bool AnimationCharacter::IsAnimationFinished() const
{
	if (animationIndex < 0) return false;

	std::shared_ptr<Model> model = const_cast<AnimationCharacter*>(this)->GetModel();
	if (!model) return false;

	const Model::Animation& animation = model->GetAnimations().at(animationIndex);
	return animationSeconds >= animation.secondsLength;
}

// 再生時間で判定する関数（特定の範囲内なら遷移OK）
bool AnimationCharacter::IsAnimationInTimeRange(float startSeconds, float endSeconds) const
{
	if (animationIndex < 0) return false;
	// 現在の再生時間がstartSeconds ～　endSecondsの間にあるかをチェック

	return (animationSeconds >= startSeconds && animationSeconds <= endSeconds);
}

// 再生時間で判定する関数（特定の範囲外なら遷移OK）
bool AnimationCharacter::IsAnimationOutTimeRange(float startSeconds, float endSeconds) const
{
	if (animationIndex < 0) return false;
	// 現在の再生時間がstartSeconds ～　endSecondsの範囲外をチェック
	return (animationSeconds <= startSeconds || animationSeconds >= endSeconds);
}

// 再生時間で判定する関数（特定の場所以降なら遷移OK）
bool AnimationCharacter::IsAnimationOutTimeRange(float StartTransition) const
{
	if (animationIndex < 0) return false;
	// 現在の再生時間がStartTransition以上かををチェック
	return (animationSeconds >= StartTransition);
}

// だんだんアニメーション遅くする関数
void AnimationCharacter::AnimationLerp(float StartSlow, float endSlow, float SlowSpeed)
{
	if (animationSeconds >= StartSlow)
	{
		// 線形補間（Lerp）の考え方で速度を計算
		// t は 0.0 (開始) から 1.0 (終了) になる
		float t = (animationSeconds - StartSlow) / (endSlow - StartSlow);
		if (t > 1.0f) t = 1.0f;

		// 通常速度 -> スローへ変化
		float newSpeed = 1.0f - (t * SlowSpeed);
		SetBaseSpeed(newSpeed);
	}
}

// このノードを起点に子ノードが上半身の子かを判断する関数
bool AnimationCharacter::IsUpperBodyNode(const Model::Node& node, const std::string& rootNodeName)
{
	const Model::Node* current = &node;
	while (current != nullptr)
	{
		if (current->name == rootNodeName) return true;
		current = current->parent; // 親をたどる
	}
	return false;
}

// 現在のアニメーションの再生時間の長さ
float AnimationCharacter::GetCurrentAnimationLength() const
{
	if (animationIndex < 0) return false;

	std::shared_ptr<Model> model = const_cast<AnimationCharacter*>(this)->GetModel();
	if (!model) return false;

	const Model::Animation& animation = model->GetAnimations().at(animationIndex);

	return animation.secondsLength;
}

// JSONファイルパスを渡して、中のノード設定を読み込む関数
void AnimationCharacter::LoadAnimationData(const std::string& jsonPath)
{
	std::ifstream file(jsonPath);
	if (!file.is_open()) return;

	nlohmann::json root;
	file >> root;

	stateConfigs.clear(); // 中身を空にしておく

	// ノードエディターで保存したnodesの設定を復元する
	if (root.count("nodes") > 0)
	{
		for (auto& node : root["nodes"])
		{
			std::string stateName = node["StateName"];

			AnimationConfig config;
			if (node.count("config") > 0)
			{
				auto& c = node["config"];
				config.animationName	= c.value("animationName", "");
				config.loop				= c.value("loop", false);
				config.useRootMotion	= c.value("useRootMotion", false);
				config.useRootMotionEx	= c.value("useRootMotionEx", false);
				config.blendTime		= c.value("blendTime", 0.2);

				config.targetBoneName = c.value("targetBoneName", "");
			}

			// Mapに登録
			stateConfigs[stateName] = config;
		}
	}
}

const AnimationConfig* AnimationCharacter::GetAnimationConfig(const std::string& stateName) const
{
	auto it = stateConfigs.find(stateName); // ※m_stateConfigs にしてる場合は m_ をつけてな
	if (it != stateConfigs.end())
	{
		return &it->second;
	}
	return nullptr; // 見つからなかったら空を返す
}
