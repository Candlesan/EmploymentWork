#include "Graze.h"


bool Graze::IsGraze(
	const DirectX::XMFLOAT3& characterPos,
	float characterYaw,
	bool isDefense,
	const DirectX::XMFLOAT3& magicPos,
	float magicRadius) const 
{
	using namespace DirectX;

	// 防御魔法を展開ていたら処理をしない
	if (isDefense) return false; 

	const XMVECTOR character = XMLoadFloat3(&characterPos);
	const XMVECTOR magic = XMLoadFloat3(&magicPos);

	// キャラから魔法ベクトル
	XMVECTOR toMagic = XMVectorSubtract(magic, character);

	// 水平で計算するからYを消す
	toMagic = XMVectorSetY(toMagic, 0.0f);

	//真上と真下に近い場合



	// 内積を取る

	// 120°以内か確認する

	return false;
}
