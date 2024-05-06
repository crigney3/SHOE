#pragma once

#include <DirectXMath.h>
#include <wrl/client.h>
#include "DXCore.h"
#include <memory>

class Texture {
protected:

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreDx11Texture;
	std::string name;
	std::string fileKey;
	AssetPathIndex assetPathIndex;

public:

	Texture();
	Texture(std::string fileKey,
		std::string name = "newTexture",
		AssetPathIndex assetPathIndex = ASSET_TEXTURE_PATH_BASIC);
	~Texture();

	virtual void SetTexture();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetDX11Texture();

	std::string GetName();
	void SetName(std::string name);

	std::string GetTextureFilenameKey();
	void SetTextureFilenameKey(std::string newFileKey);

	AssetPathIndex GetAssetPathIndex();
	void SetAssetPathIndex(AssetPathIndex pathIndex);

};

class DX11Texture : public Texture {
public:
	DX11Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture,
				std::string fileKey,
				std::string name = "newTexture",
				AssetPathIndex assetPathIndex = ASSET_TEXTURE_PATH_BASIC);
	~DX11Texture();

	void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture);
};

class DX12Texture : public Texture {
public:
	// Make these actually DX12 data later
	DX12Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture,
		std::string fileKey,
		std::string name = "newTexture",
		AssetPathIndex assetPathIndex = ASSET_TEXTURE_PATH_BASIC);
	~DX12Texture();

	void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture();
};