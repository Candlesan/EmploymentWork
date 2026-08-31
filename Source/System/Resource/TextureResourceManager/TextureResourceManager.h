#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <string>
#include <unordered_map>

class TextureResourceManager
{
public:
	static TextureResourceManager& Instance()
	{
		static TextureResourceManager instance;
		return instance;
	}

	// パスを渡すとキャッシュされたSRVを返す
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetOrLoad(ID3D11Device* device, const std::string& path);

	void Clear();

private:
	TextureResourceManager() = default;
	~TextureResourceManager() = default;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> cache;
};