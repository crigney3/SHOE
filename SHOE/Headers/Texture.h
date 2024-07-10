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

	// Setting a texture as "temp" means it will not be saved.
	// Currently used to prevent particle textures from saving
	// as individual textures instead of folders.
	bool tempTexture;

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

	bool IsTextureTemp();
	void SetIsTextureTemp(bool tempState);

};

class DX11Texture : public Texture {
public:
	DX11Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture,
				std::string fileKey,
				std::string name = "newTexture",
				AssetPathIndex assetPathIndex = ASSET_TEXTURE_PATH_BASIC);
	~DX11Texture();

	void SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture);

	D3D11_TEXTURE2D_DESC GetTextureDesc();
	void SetTextureDesc(D3D11_TEXTURE2D_DESC newDesc);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> GetInternalTexture();
	void SetInternalTexture(Microsoft::WRL::ComPtr<ID3D11Texture2D> newInternalTexture);

protected:
	D3D11_TEXTURE2D_DESC textureDesc;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> internalTexture;
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