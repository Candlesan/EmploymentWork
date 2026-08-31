#include "TextureResourceManager.h"
#include "System/Graphic/GpuResourceUtils.h"


Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TextureResourceManager::GetOrLoad(ID3D11Device* device, const std::string& path)
{
    // パスが無ければnullptrを返す
    if (path.empty()) return nullptr;

    // キャッシュ内を「path」で検索する
    auto it = cache.find(path);
    if (it != cache.end())
    {
        return it->second; // 既存のSRVを返す
    }

    // 初回のみディスクから読み込み
    D3D11_TEXTURE2D_DESC desc;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    HRESULT hr = GpuResourceUtils::LoadTexture(device, path.c_str(), srv.GetAddressOf(), &desc);
    if (FAILED(hr))
    {
        return nullptr;
    }

    // キャッシュに登録
    cache[path] = srv;

    return srv;
}

void TextureResourceManager::Clear()
{
    cache.clear();
}