//-------------------------------------------- 
// シャドウマップ参照用情報の計算 
//-------------------------------------------- 
// worldPosition  : ワールド座標 
// lightViewProjection : ライトビュープロジェクション行列 
// 返す値    : シャドウマップ参照用のUV座標及び深度情報 
float3 CalcShadowTexcoord(float3 worldPosition, matrix lightViewProjection)
{
	// クリップ空間の座標に変換
    float4 worldViewProjectionPosition = mul(float4(worldPosition, 1), lightViewProjection);

	// 透視除算してNDC座標に変換
    worldViewProjectionPosition /= worldViewProjectionPosition.w;

	// NDC座標のXY座標をUV座標に変換
	// Z値はすでに0～1の範囲に変換されているので、何もしなくて良い
    worldViewProjectionPosition.y = -worldViewProjectionPosition.y;
    worldViewProjectionPosition.xy = 0.5f * worldViewProjectionPosition.xy + 0.5f;
    return worldViewProjectionPosition.xyz;
}

//-------------------------------------------- 
// シャドウマップから深度値を取得して影かどうかを渡す 
//-------------------------------------------- 
// tex    : シャドウマップ 
// samplerState  : サンプラステート 
// shadowTexcoord : シャドウマップ参照用情報 
// shadowColor  : 影の色 
// shadowBias  : 深度比較用のオフセット値 
// 返す値   : 影かどうか 
float3 CalcShadowColor(Texture2D tex, SamplerState samplerState, float3 shadowTexcoord, float3 shadowColor, float shadowBias)
{
	// シャドウマップから深度値取得
    float depth = tex.Sample(samplerState, shadowTexcoord.xy).r;

	// 深度値を比較して影かどうかを判定する
    float shadow = step(shadowTexcoord.z - depth, shadowBias);

    return lerp(shadowColor, 1, shadow);
}
