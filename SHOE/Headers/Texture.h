#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include "DXCore.h"
#include <memory>

class Texture {
private:

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture;
	std::string name;
	std::string fileKey;
	AssetPathIndex assetPathIndex;

public:

	Texture();
	Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture,
			std::string fileKey,
			std::string name = "newTexture",
			AssetPathIndex assetPathIndex = ASSET_TEXTURE_PATH_BASIC);
	~Texture();

	void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture();

	std::string GetName();
	void SetName(std::string name);

	std::string GetTextureFilenameKey();
	void SetTextureFilenameKey(std::string newFileKey);

	AssetPathIndex GetAssetPathIndex();
	void SetAssetPathIndex(AssetPathIndex pathIndex);

};