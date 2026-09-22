#pragma once
#include "json.hpp"
#include "System/UI/UIEditor/UIEditor.h"

#include <DirectXMath.h>
#include <d3d11.h>
#include <string>
#include <memory>


class UILayout
{
public:
	// 読み込み・保存
	bool Load(ID3D11Device* device, const std::string& filepath);
	bool Save(const std::string& filepath) const;
	void Clear(); // 新規作成

	void Update(float elapsedTime); // World行列・Colorを計算
	void Render(ID3D11DeviceContext* dc); 

	UIElement* Find(const std::string& name);
	UIElement* GetRoot() { return root.get(); }
private:
};