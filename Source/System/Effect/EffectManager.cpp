#include "System/Graphic/Graphics.h"
#include "System/Effect/EffectManager.h"
#include "System/Effect/Effect.h"

	
// コンストラクタ
EffectManager::EffectManager()
{
}

// デストラクタ
EffectManager::~EffectManager()
{
	// 中身は空っぽでOK（unique_ptr が勝手に中身を解放してくれるため）
}

// 唯一のインスタンス
EffectManager& EffectManager::Instance()
{
	static EffectManager instance;
	return instance;
}

//初期化
void EffectManager::Initialize()
{
	Graphics& graphics = Graphics::Instance();

	//Effekseerレンダラ生成
	effekseerRenderer = EffekseerRendererDX11::Renderer::Create(graphics.GetDevice(),
		graphics.GetDeviceContext(), 2048);

	//Effekseerマネージャー生成
	effekseerManager = Effekseer::Manager::Create(2048);

	//Effekseerレンダラの各種設定（特別なカスタマイズをしない場合は定型的に以下の設定でOK）
	effekseerManager->SetSpriteRenderer(effekseerRenderer->CreateSpriteRenderer());
	effekseerManager->SetRibbonRenderer(effekseerRenderer->CreateRibbonRenderer());
	effekseerManager->SetRingRenderer(effekseerRenderer->CreateRingRenderer());
	effekseerManager->SetTrackRenderer(effekseerRenderer->CreateTrackRenderer());
	effekseerManager->SetModelRenderer(effekseerRenderer->CreateModelRenderer());
	//Effekseer内でのローダーの設定
	effekseerManager->SetTextureLoader(effekseerRenderer->CreateTextureLoader());
	effekseerManager->SetModelLoader(effekseerRenderer->CreateModelLoader());
	effekseerManager->SetMaterialLoader(effekseerRenderer->CreateMaterialLoader());

	//Effekseerを左手座標系で計算する
	effekseerManager->SetCoordinateSystem(Effekseer::CoordinateSystem::LH);
}

//終了化
void EffectManager::Finalize()
{

}

//更新処理
void EffectManager::Update(float elapsedTime)
{
	//エフェクト更新処理（引数にはフレームの経過時間を渡す）
	effekseerManager->Update(elapsedTime * 60.0f);
}

//描画処理
void EffectManager::Render(const DirectX::XMFLOAT4X4& view, const DirectX::XMFLOAT4X4& projection)
{
	// ビュー＆プロジェクション行列をEffekseerレンダラに設定 
	effekseerRenderer->SetCameraMatrix(*reinterpret_cast<const Effekseer::Matrix44*>(&view));
	effekseerRenderer->SetProjectionMatrix(*reinterpret_cast<const
		Effekseer::Matrix44*>(&projection));
	//Effekseer描画開始
	effekseerRenderer->BeginRendering();

	//Effekseer描画実行
	//マネージャー単位で描画するので描画順を制御する場合はマネージャーを複数個作成し、
	//Draw()関数を実行する順序で制御出来そう
	effekseerManager->Draw();

	//Effekseer描画終了
	effekseerRenderer->EndRendering();
}

Effect* EffectManager::GetOrLoadEffect(const std::string& fileName)
{
	// なかったらnullを返す
	if (fileName.empty()) return nullptr;

	// 既に読み込まれているかを確認する
	auto it = effectCache.find(fileName);
	if (it != effectCache.end())
	{
		// 見つかったらそれを使いまわす
		return it->second.get();
	}

	// 見つからなかった場合は新しく読み込んで登録する
	effectCache[fileName] = std::make_unique<Effect>(fileName.c_str());

	// 登録したものを返す
	return effectCache[fileName].get();
}
