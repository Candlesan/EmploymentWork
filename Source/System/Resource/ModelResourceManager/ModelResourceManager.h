#pragma once
#include <string>
#include <memory>
#include <d3d11.h>
#include <unordered_map>

// 前方宣言
class Model;

// 読み込み時に登録して使い回せるようにするクラス
class ModelResourceManager
{
public:
	// シングルトン
	static ModelResourceManager& Instance()
	{
		static ModelResourceManager instance;
		return instance;
	}

	// パスをわたしたら、あったら返してなかったら読み込んで返す
	std::shared_ptr<Model> GetOrLoad(ID3D11Device* device, const std::string& path);

	// シーン切り替え時に使う消去関数
	void Clear();
private:
	ModelResourceManager() {};
	~ModelResourceManager() {};

	std::unordered_map<std::string, std::shared_ptr<Model>> cache; // キー：名前/値：モデル
};

