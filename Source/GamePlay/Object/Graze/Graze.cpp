#include "Graze.h"
#include <DirectXMath.h>


// 弾をかすめた時のクラス
class Graze
{
public:

private:
	float grazeRadius = 0.5f; // グレイズの半径
	DirectX::XMFLOAT3 grazePosition = { 0.0f, 0.0f, 0.0f };
};
