#include "UICanvas.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/Sprite/Sprite.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"

#include <fstream>
#include <imgui.h>


// ヘルパー関数
static nlohmann::json ToJson(float v) { return v; }
static nlohmann::json ToJson(const DirectX::XMFLOAT2& v) { return { v.x, v.y }; }

static void FromJson(const nlohmann::json& j, float& v) { v = j.get<float>(); }
static void FromJson(const nlohmann::json& j, DirectX::XMFLOAT2& v) { v = { j[0], j[1] }; }

template<typename T>
static nlohmann::json SaveTween(const UITween<T>& tween)
{
	return {
		{"from", ToJson(tween.from)},
		{"to", ToJson(tween.to)},
		{"duration", tween.duration},
		{"ease", static_cast<int>(tween.ease)},
		{"loop", static_cast<int>(tween.loop)},
		{"loopCount", tween.loopCount},
		{"enabled", tween.enabled},
	};
}

// Tween1個分の設定を読み込み(ない項目は今の値のまま)
template<typename T>
static void LoadTween(const nlohmann::json& j, UITween<T>& tween)
{
	if (j.find("from") != j.end())      FromJson(j["from"], tween.from);
	if (j.find("to") != j.end())        FromJson(j["to"], tween.to);
	tween.duration = j.value("duration", tween.duration);
	tween.ease = static_cast<EasingType>(j.value("ease", static_cast<int>(tween.ease)));
	tween.loop = static_cast<LoopType>(j.value("loop", static_cast<int>(tween.loop)));
	tween.loopCount = j.value("loopCount", -1);
	tween.enabled = j.value("enabled", false);
}



// 初期化
void UICanvas::Initialize(ID3D11Device* device)
{
	this->device = device;
}

// 更新処理
void UICanvas::Update(float elapsedTime)
{
	for (auto& e : canvas.elements)
	{
		UpdateElement(e.get(), elapsedTime);
	}
}
void UICanvas::UpdateElement(UIElement* e, float elapsedTime)
{
	// 自分自身を更新
	if (e->fadeAnim.playing)
	{
		e->fadeAnim.Update(elapsedTime);
		e->fadeAlpha = e->fadeAnim.GetValue();
	}

	if (e->moveAnim.playing)
	{
		e->moveAnim.Update(elapsedTime);
		e->animPosition = e->moveAnim.GetValue();
	}

	if (e->rotationAnim.playing)
	{
		e->rotationAnim.Update(elapsedTime);
		e->rotationValue = e->rotationAnim.GetValue();
	}

	if (e->scaleAnim.playing)
	{
		e->scaleAnim.Update(elapsedTime);
		e->scaleValue = e->scaleAnim.GetValue();
	}

	// 子を更新する
	for (auto& child : e->children)
	{
		UpdateElement(child.get(), elapsedTime);
	}
}

// 描画処理
void UICanvas::Render(ID3D11DeviceContext* dc)
{
	RenderState* renderState = Graphics::Instance().GetRenderState();

	// 半透明で描く
	dc->OMSetBlendState(renderState->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
	// 深度は見ないので書かない(常に配列の順番通りに描く)
	dc->OMSetDepthStencilState(renderState->GetDepthStencilState(DepthState::NoTestNoWrite), 0);
	// <テクスチャのサンプラーをセットする
	ID3D11SamplerState* sampler = renderState->GetSamplerState(SamplerState::LinearClamp);
	dc->PSSetSamplers(0, 1, &sampler);

	for (auto& e : canvas.elements)
	{
		// eは今処理をしている自分
		RenderElement(dc, e.get(), DirectX::XMMatrixIdentity(), { 1, 1, 1, 1 });
	}
}
void UICanvas::RenderElement(ID3D11DeviceContext* dc, UIElement* e, const DirectX::XMMATRIX& parentWorld, const DirectX::XMFLOAT4& parentColor)
{
	// 非表示なら子も含めて描かない
	if (!e->visible) return;

	// 親の四角形の中のanhcorの位置
	DirectX::XMFLOAT2 anchorPos;
	if (e->parent)
	{
		// 親の(0,0)は親のpivotの位置なので、pivotからの距離にする
		anchorPos.x = (e->anchor.x - e->parent->pivot.x) * e->parent->size.x;
		anchorPos.y = (e->anchor.y - e->parent->pivot.y) * e->parent->size.y;
	}
	else
	{
		// ルートの親はCanvas(左上(0,0))
		anchorPos.x = e->anchor.x * canvas.referenceResolution.x;
		anchorPos.y = e->anchor.y * canvas.referenceResolution.y;
	}

	// ローカル行列を作る
	DirectX::XMMATRIX Scale = DirectX::XMMatrixScaling(e->scale.x * e->scaleValue, e->scale.y * e->scaleValue, 1.0f);
	DirectX::XMMATRIX Angle = DirectX::XMMatrixRotationZ(DirectX::XMConvertToRadians(e->rotation + e->rotationValue)); // Z軸しか2Dは回転しやんからXMMatrixRotationZ

	DirectX::XMFLOAT2 pos = e->moveAnim.enabled ? e->animPosition : e->position;
	DirectX::XMMATRIX Position = DirectX::XMMatrixTranslation(
		anchorPos.x + pos.x,
		anchorPos.y + pos.y,
		0.0f
	);
	DirectX::XMMATRIX world = Scale * Angle * Position * parentWorld;
	DirectX::XMStoreFloat4x4(&e->worldMatrix, world);

	e->worldColor = {
	parentColor.x * e->color.x,
	parentColor.y * e->color.y,
	parentColor.z * e->color.z,
	parentColor.w * e->color.w * e->fadeAlpha
	};

	if (!e->texturePath.empty() && e->sprite)
	{
		// 表示サイズ
		float w = e->size.x;
		float h = e->size.y;

		// 基準点(画像の中のどこか, px)
		// pivot×表示サイズ
		float px = e->pivot.x * w;
		float py = e->pivot.y * h;

		// 基準点を原点にした4隅(左上、右上、左下、右下)
		DirectX::XMFLOAT2 cornners[4] = {
			{-px, -py},
			{w - px, -py},
			{-px, h - py},
			{w - px, h - py}
		};

		// 4隅をワールド行列で変換（回転・拡大・移動が全部かかる）
		for (auto& p : cornners)
		{
			DirectX::XMVECTOR v = DirectX::XMLoadFloat2(&p);
			v = DirectX::XMVector2TransformCoord(v, world);
			DirectX::XMStoreFloat2(&p, v);
		}

		// 切り抜き範囲（sw,shが0なら画像全体を使う）
		DirectX::XMFLOAT4 uv = e->uvRect;
		if (uv.z <= 0.0f || uv.w <= 0.0f)
		{
			DirectX::XMFLOAT2 texSize = e->sprite->GetTextureSize();
			uv = { 0.0f, 0.0f, texSize.x, texSize.y };
		}

		e->sprite->Render(dc,
			cornners,
			0.0f,
			uv.x, uv.y, uv.z, uv.w,
			e->worldColor);
	}

	// 子を描く(自分の後に描くので子が手前に出る)
	for (auto& child : e->children)
	{
		RenderElement(dc, child.get(), world, e->worldColor);
	}
}

// GUI描画
void UICanvas::DrawGUI()
{
}

// 保存
void UICanvas::Save(const std::string& path)
{
	using json = nlohmann::json;
	json j;

	j["referenceResolution"] = { canvas.referenceResolution.x, canvas.referenceResolution.y };

	// 要素
	j["elements"] = json::array();
	for (auto& e : canvas.elements)
	{
		j["elements"].push_back(SaveElement(e.get()));
	}

	std::ofstream file(path);
	if (file.is_open())
	{
		file << j.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
	}
}
nlohmann::json UICanvas::SaveElement(const UIElement* e)
{
	using json = nlohmann::json;

	// 自分の中身
	json j = {
			{"name",e->name},
			{"texturePath", e->texturePath},
			{"uvRect", { e->uvRect.x, e->uvRect.y, e->uvRect.z, e->uvRect.w }},
			{"position", {e->position.x, e->position.y}},
			{"rotation", e->rotation},
			{"size", {e->size.x, e->size.y}},
			{"scale", {e->scale.x, e->scale.y}},
			{"pivot", {e->pivot.x, e->pivot.y}},
			{"anchor", {e->anchor.x, e->anchor.y}},
			{"color", { e->color.x, e->color.y, e->color.z, e->color.w }},
			{"visible", e->visible},

	};

	j["animations"] = {
	{"fade", SaveTween(e->fadeAnim)},
	{"move", SaveTween(e->moveAnim)},
	{"rotation", SaveTween(e->rotationAnim)},
	{"scale", SaveTween(e->scaleAnim)},
	};

	// 子は子自身に保存してもらう
	j["children"] = json::array();
	for (auto& child : e->children)
	{
		j["children"].push_back(SaveElement(child.get()));
	}

	return j;
}

// 読み込み
void UICanvas::Load(const std::string& path)
{
	using json = nlohmann::json;
	std::ifstream file(path);
	if (!file.is_open()) return;

	json j;
	file >> j;

	if (j.find("referenceResolution") != j.end()) canvas.referenceResolution = { j["referenceResolution"][0], j["referenceResolution"][1] };

	// 配列が存在する場合全て読み込む
	if (j.find("elements") != j.end() && j["elements"].is_array())
	{
		// 既存のElementを削除してリセット！
		canvas.elements.clear();

		for (const auto& je : j["elements"])   // je = 要素1個分のJSON
		{
			// ルートなので親はいない
			canvas.elements.emplace_back(LoadElement(je, nullptr));
		}
	}
}
std::unique_ptr<UIElement> UICanvas::LoadElement(const nlohmann::json& je, UIElement* parent)
{
	auto element = std::make_unique<UIElement>();
	element->parent = parent;

	// 自分自身の中身
	element->name = je.value("name", "");
	element->texturePath = je.value("texturePath", "");
	if (je.find("uvRect") != je.end())   element->uvRect = { je["uvRect"][0], je["uvRect"][1], je["uvRect"][2], je["uvRect"][3] };
	if (je.find("position") != je.end()) element->position = { je["position"][0], je["position"][1] };
	element->rotation = je.value("rotation", 0.0f);
	if (je.find("size") != je.end())     element->size = { je["size"][0], je["size"][1] };
	if (je.find("scale") != je.end())    element->scale = { je["scale"][0], je["scale"][1] };
	if (je.find("pivot") != je.end())    element->pivot = { je["pivot"][0], je["pivot"][1] };
	if (je.find("anchor") != je.end())   element->anchor = { je["anchor"][0], je["anchor"][1] };
	if (je.find("color") != je.end())    element->color = { je["color"][0], je["color"][1], je["color"][2], je["color"][3] };
	element->visible = je.value("visible", true);   // ← 初期値は true に

	// テクスチャを読み直す
	if (!element->texturePath.empty())
	{
		element->sprite = std::make_shared<Sprite>(device, element->texturePath.c_str());
	}

	if (je.find("animations") != je.end())
	{
		const auto& ja = je["animations"];
		if (ja.find("fade") != ja.end())     LoadTween(ja["fade"], element->fadeAnim);
		if (ja.find("move") != ja.end())     LoadTween(ja["move"], element->moveAnim);
		if (ja.find("rotation") != ja.end()) LoadTween(ja["rotation"], element->rotationAnim);
		if (ja.find("scale") != ja.end())    LoadTween(ja["scale"], element->scaleAnim);
	}

	// 読み込んだ設定で、アニメーションの開始の状態にしておく
	ResetAnimation(element.get());

	// 子は子自身に読み込んでもらう
	if (je.find("children") != je.end() && je["children"].is_array())
	{
		for (const auto& jc : je["children"])
		{
			element->children.emplace_back(LoadElement(jc, element.get()));
		}
	}

	return element;
}

// UIを探す関数
UIElement* UICanvas::Find(const std::string& name)
{
	for (auto& e : canvas.elements)
	{
		UIElement* found = FindElement(e.get(), name);
		if (found) return found;
	}

	// どこにもなかった
	return nullptr;
}
UIElement* UICanvas::FindElement(UIElement* e, const std::string& name)
{
	// 自分なら自分を返す
	if (e->name == name) return e;

	// 自分じゃなければ子を探す
	for (auto& child : e->children)
	{
		UIElement* found = FindElement(child.get(), name);
		if (found) return found;
	}

	// 子にもいなかった
	return nullptr;
}

// 再生関数
void UICanvas::Play(UIElement* e)
{
	if (!e) return; // UIが見つからなかったら処理をしない

	if (IsPlaying(e)) return; // 再生中なら2重呼び出しをしないようにする

	// 自分自身を再生する
	e->moveAnim.Play();
	e->rotationAnim.Play();
	e->scaleAnim.Play();
	e->fadeAnim.Play();

	for (auto& child : e->children)
	{
		Play(child.get());
	}
}

// 全て再生する
void UICanvas::AllPlay()
{
	for (auto& e : canvas.elements)
	{
		Play(e.get());
	}
}

// 停止
void UICanvas::Stop(UIElement* e)
{
	if (!e) return;
	e->moveAnim.Stop();
	e->rotationAnim.Stop();
	e->scaleAnim.Stop();
	e->fadeAnim.Stop();
}

// 再生中か
bool UICanvas::IsPlaying(UIElement* e)
{
	if (!e) return false;
	bool isPlaying = (e->moveAnim.playing || e->rotationAnim.playing ||
		e->scaleAnim.playing || e->fadeAnim.playing);
	return isPlaying;
}

// 要素のアニメーションを全部止めて、見た目を元に戻す
void UICanvas::ResetAnimation(UIElement* e)
{
	// 全部のTweenを止める(from・to・時間などの設定は残る)
	e->fadeAnim.Stop();
	e->moveAnim.Stop();
	e->rotationAnim.Stop();
	e->scaleAnim.Stop();

	// アニメーションの結果を初期値に戻す
	e->fadeAlpha = e->fadeAnim.enabled ? e->fadeAnim.from : 1.0f;
	e->animPosition = e->moveAnim.enabled ? DirectX::XMFLOAT2(e->moveAnim.from.x, e->moveAnim.from.y) : DirectX::XMFLOAT2(0.0f, 0.0f);
	e->rotationValue = e->rotationAnim.enabled ? e->rotationAnim.from : 0.0f;
	e->scaleValue = e->scaleAnim.enabled ? e->scaleAnim.from : 1.0f;
}
