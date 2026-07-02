#pragma once
#include <DirectXMath.h>

class IMagicMove
{
public:
	virtual ~IMagicMove() = default;

	// 弾の座標や向きを更新するための関数
	//参照渡しにして、弾本体の変数を直接書き換えられるようにする
	virtual void UpdateMove(
		DirectX::XMFLOAT3& position,
		DirectX::XMFLOAT3& direction,
		float speed,
		float elapsedTime) = 0;
};