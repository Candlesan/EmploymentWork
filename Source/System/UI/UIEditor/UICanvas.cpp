#include "UICanvas.h"
#include "System/Graphic/Graphics.h"
#include "System/Resource/Sprite/Sprite.h"
#include "System/Resource/TextureResourceManager/TextureResourceManager.h"

#include <fstream>
#include <imgui.h>
#include <algorithm>
#include <cassert>
#include <cmath>


// ヘルパー関数
static nlohmann::json ToJson(float v) { return v; }
static nlohmann::json ToJson(const DirectX::XMFLOAT2& v) { return { v.x, v.y }; }

static void FromJson(const nlohmann::json& j, float& v) { v = j.get<float>(); }
static void FromJson(const nlohmann::json& j, DirectX::XMFLOAT2& v) { v = { j[0], j[1] }; }

// 入力方向で次の位置を決める
static int StepIndex(int pos, int delta, int count, bool loop)
{
	int next = pos + delta;
	// ループするかしないか
	if (loop)
	{
		next = (next % count + count) % count; // 端から反対側へ
	}
	else
	{
		next = std::clamp(next, 0, count - 1); // 端で止まる
	}

	return next;
}

// UITweenの保存
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

// UIAnimの保存
static nlohmann::json SaveAnimSet(const UIAnimSet& set)
{
	return {
		{"fade",     SaveTween(set.fade)},
		{"move",     SaveTween(set.move)},
		{"rotation", SaveTween(set.rotation)},
		{"scale",    SaveTween(set.scale)},
	};
}

static void LoadAnimSet(const nlohmann::json& j, UIAnimSet& set)
{
	if (j.find("fade") != j.end())     LoadTween(j["fade"], set.fade);
	if (j.find("move") != j.end())     LoadTween(j["move"], set.move);
	if (j.find("rotation") != j.end()) LoadTween(j["rotation"], set.rotation);
	if (j.find("scale") != j.end())    LoadTween(j["scale"], set.scale);
}


// 初期化
void UICanvas::Initialize(ID3D11Device* device)
{
	this->device = device;
	uifillRenderer.Initialize(device);
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
	if (e->anim.fade.playing)
	{
		e->anim.fade.Update(elapsedTime);
		e->fadeAlpha = e->anim.fade.GetValue();
	}

	if ((e->anim.move.playing))
	{
		e->anim.move.Update(elapsedTime);
		e->animPosition = e->anim.move.GetValue();
	}

	if (e->anim.rotation.playing)
	{
		e->anim.rotation.Update(elapsedTime);
		e->rotationValue = e->anim.rotation.GetValue();
	}

	if (e->anim.scale.playing)
	{
		e->anim.scale.Update(elapsedTime);
		e->scaleValue = e->anim.scale.GetValue();
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

	DirectX::XMFLOAT2 pos = e->anim.move.enabled ? e->animPosition : e->position;
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

		DirectX::XMFLOAT2 tex = e->sprite->GetTextureSize();
		uifillRenderer.Apply(dc,
			{ uv.x / tex.x, uv.y / tex.y, uv.z / tex.x, uv.w / tex.y },
			e->fillAmount, e->fillMethod);

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
			{"fillMethod", static_cast<int>(e->fillMethod)},
			{"focusIndex",      e->focusIndex},
			{"focusHorizontal", e->focusHorizontal},
			{"focusLoop",       e->focusLoop}, 
	};

	j["animations"] = SaveAnimSet(e->anim);
	j["focusAnim"] = SaveAnimSet(e->focusAnim);
	j["unfocusAnim"] = SaveAnimSet(e->unfocusAnim);

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
		tabFocus.Clear();
		itemFocus.Clear();
		tabFocus.pos = -1;
		itemFocus.pos = -1;


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
	element->visible = je.value("visible", true);
	element->fillMethod = static_cast<FillMethod>(je.value("fillMethod", FillMethod::None));
	element->focusIndex = je.value("focusIndex", -1);
	element->focusHorizontal = je.value("focusHorizontal", false);
	element->focusLoop = je.value("focusLoop", false);

	// テクスチャを読み直す
	if (!element->texturePath.empty())
	{
		element->sprite = CreateSprite(element->texturePath.c_str());;
	}

	if (je.find("animations") != je.end()) LoadAnimSet(je["animations"], element->anim);
	if (je.find("focusAnim") != je.end()) LoadAnimSet(je["focusAnim"], element->focusAnim);
	if (je.find("unfocusAnim") != je.end()) LoadAnimSet(je["unfocusAnim"], element->unfocusAnim);

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

	if (e->anim.IsPlaying()) return; // 再生中なら2重呼び出しをしないようにする

	// 自分自身を再生する
	e->anim.Play();

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
	e->anim.Stop();
	e->anim.Stop();
	e->anim.Stop();
	e->anim.Stop();
}

// 再生中か
bool UICanvas::IsPlaying(UIElement* e)
{
	if (!e) return false;
	bool isPlaying = (e->anim.move.playing || e->anim.rotation.playing ||
		e->anim.scale.playing || e->anim.fade.playing);
	return isPlaying;
}

// 要素のアニメーションを全部止めて、見た目を元に戻す
void UICanvas::ResetAnimation(UIElement* e)
{
	// 全部のTweenを止める(from・to・時間などの設定は残る)
	e->anim.Stop();
	e->anim.Stop();
	e->anim.Stop();
	e->anim.Stop();

	// アニメーションの結果を初期値に戻す
	e->fadeAlpha = e->anim.fade.enabled ? e->anim.fade.from : 1.0f;
	e->animPosition = e->anim.move.enabled ? DirectX::XMFLOAT2(e->anim.move.from.x, e->anim.move.from.y) : DirectX::XMFLOAT2(0.0f, 0.0f);
	e->rotationValue = e->anim.rotation.enabled ? e->anim.rotation.from : 0.0f;
	e->scaleValue = e->anim.scale.enabled ? e->anim.scale.from : 1.0f;
}

std::shared_ptr<Sprite> UICanvas::CreateSprite(const std::string& path)
{
	// キャッシュ済みならそれを使い回す
	auto srv = TextureResourceManager::Instance().GetOrLoad(device, path);
	auto sprite = std::make_shared<Sprite>(device, srv);
	sprite->SetSpritePS(uifillRenderer.GetPixelShader());
	return sprite;
}

// どのパネルを選ぶか決めて先頭にフォーカスする
void UICanvas::SetPanel(const std::string& name)
{
	Select(itemFocus, -1); // 前のハイライトを消す
	Select(tabFocus, -1);
	Collect(tabFocus, Find(name)); // パネルの子を集める
	SelectTab(tabFocus.items.empty() ? -1 : 0);
}

// 入力された時の更新
void UICanvas::UpdateNavigation(const GamePad& pad)
{
	int dx = 0, dy = 0;

	// 十字キー
	GamePadButton down = pad.GetButtonDown();
	if (down & GamePad::BTN_LEFT)	dx = -1;
	if (down & GamePad::BTN_RIGHT)	dx =  1;
	if (down & GamePad::BTN_UP)		dy = -1;
	if (down & GamePad::BTN_DOWN)	dy =  1;

	// スティック
	float lx = pad.GetAxisLX();
	float ly = pad.GetAxisLY();
	const float on = 0.5f; // これ以上倒したら入力
	const float off = 0.3f; // これよりもどったら倒したら中央

	if (stickNeutral)
	{
		if (lx < -on) { dx = -1; stickNeutral = false; }
		else if (lx > on) { dx = 1; stickNeutral = false; }
		else if (ly > on) { dy = -1; stickNeutral = false; } // 上に倒すと LY はプラス
		else if (ly < -on) { dy = 1; stickNeutral = false; }
	}
	else if (fabsf(lx) < off && fabsf(ly) < off)
	{
		stickNeutral = true;
	}

	Navigate(dx, dy);
}

// 入力方向に動かす
void UICanvas::Navigate(int dx, int dy)
{
	if (!tabFocus.owner) return;

	// パネルの向きで、使う入力を決める
	bool horizontal = tabFocus.owner->focusHorizontal;
	int tabDelta  = horizontal ? dx : dy;
	int itemDelta = horizontal ? dy : dx;

	// タブの位置の切り替え
	if (tabDelta != 0 && !tabFocus.items.empty())
	{
		int next = StepIndex(tabFocus.pos, tabDelta, tabFocus.items.size(), tabFocus.owner->focusLoop);
		if (next != tabFocus.pos) SelectTab(next); // 端で止まらないようにループさせる
		return;
	}

	// アイテムの位置の切り替え
	if (itemDelta != 0 && !itemFocus.items.empty())
	{
		int next = StepIndex(itemFocus.pos, itemDelta, itemFocus.items.size(), itemFocus.owner->focusLoop);
		if (next != itemFocus.pos) Select(itemFocus, next); // 端で止まらないようにループさせる
	}
}

// ownerの子から選べる物を集める
void UICanvas::Collect(FocusGroup& g, UIElement* owner)
{
	g.Clear();
	g.owner = owner;
	if (!owner) return;

	for (auto& child : owner->children)
	{
		int i = child->focusIndex;
		if (i < 0 || !child->visible) continue; // ボタンとして使ってなくて非表示の時

		if (i >= static_cast<int>(g.items.size())) g.items.resize(i + 1, nullptr); // 長さが足りないとき長さを伸ばす
		assert(!g.items[i] && "focusIndexが重複してる");
		g.items[i] = child.get();
	}

	for (UIElement* e : g.items) assert(e && "focusIndexが飛んでいる");
}

// ハイライトを付け替えて位置を変える
void UICanvas::Select(FocusGroup& g, int newPos)
{
	SetHighlight(g.Focused(), false); // 今のハイライトを消す
	g.pos = newPos;
	SetHighlight(g.Focused(), true); // 新しいボタンのハイライトを出す
}

// タブを変えた時に中の物を集め直す
void UICanvas::SelectTab(int newPos)
{
	Select(itemFocus, -1);								 // 古いタブのハイライトを消す
	Select(tabFocus, newPos);							 // タブを付け替える
	Collect(itemFocus, tabFocus.Focused());				 // 新しいタブの中を集める
	Select(itemFocus, itemFocus.items.empty() ? -1 : 0); // 先頭を光らせる
}

// ハイライトを切り替える
void UICanvas::SetHighlight(UIElement* e, bool on)
{
	if (!e) return;

	// 選ばれたタブを表示
	for (auto& c : e->children)
	{
		if (c->name == "Highlight")
		{
			c->visible = on;
			if (on)
			{
				Play(c.get()); // ハイライト再生
			}
			else
			{
				ResetAnimation(c.get()); // アニメーション停止
			}
		}
		else if (c->name == "info")
		{
			c->visible = on;   // 選ばれてるボタンの info だけ表示
		}
		else if (c->focusIndex >= 0) // 選べる子（＝タブの中の魔法ボタン）
		{
			c->visible = on;
		}
	}
}