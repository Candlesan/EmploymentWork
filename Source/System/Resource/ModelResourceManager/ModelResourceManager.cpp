#include "ModelResourceManager.h"
#include "System/Resource/Model/Model.h"


std::shared_ptr<Model> ModelResourceManager::GetOrLoad(ID3D11Device* device, const std::string& path)
{
    // リストにこのモデルがあるかを探す
    auto it = cache.find(path);

    // もしあるならそのまま返す
    if (it != cache.end())
    {
        return it->second; // cacheはキーがstring、値がstd::shared_ptr<Model>やからsecondを返す
    }

    // なかったら新しく読み込み
    std::shared_ptr<Model> newModel = std::make_shared<Model>(device, path.c_str());

    // リストに登録する
    cache[path] = newModel; // pathをキーにするのがいいのかわからんけど一旦これでいく

    // 作ったものを返す
    return newModel;
}

void ModelResourceManager::Clear()
{
    cache.clear();
}
