#pragma once

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "DXCore.h"
#include "SimpleShader.h"
#include <memory>
#include <vector>

enum PBRTextureTypes {
	ALBEDO,
	NORMAL,
	METAL,
	ROUGH
};

class Material
{
private:
	float uvTiling;
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimplePixelShader> pixShader;
	std::shared_ptr<SimplePixelShader> refractivePixShader;
	std::shared_ptr<SimpleVertexShader> vertShader;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap;

	bool enabled;
	std::string name;

	bool transparent;
	bool refractive;

	float indexOfRefraction;
	float refractionScale;

	std::string albedoFileKey;
	std::string normalsFileKey;
	std::string metalFileKey;
	std::string roughnessFileKey;

public:
	Material(DirectX::XMFLOAT4 tint,
			 std::shared_ptr<SimplePixelShader> pix,
			 std::shared_ptr<SimpleVertexShader> vert,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
			 Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
			 Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap,
			 std::string name = "material",
			 bool transparent = false,
			 bool refractive = false);
	~Material();

	DirectX::XMFLOAT4 GetTint();
	void SetTint(DirectX::XMFLOAT4 tint);
	std::shared_ptr<SimplePixelShader> GetPixShader();
	std::shared_ptr<SimpleVertexShader> GetVertShader();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetNormalMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetMetalMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRoughMap();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture();

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSamplerState();
	void SetSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> texSamplerState);

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetClampSamplerState();
	void SetClampSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSamplerState);

	float GetTiling();
	void SetTiling(float uv);

	std::string GetName();
	void SetName(std::string name);

	void SetTransparent(bool transparent);
	bool GetTransparent();

	void SetRefractive(bool refractive);
	bool GetRefractive();

	void SetIndexOfRefraction(float index);
	float GetIndexOfRefraction();

	void SetRefractionScale(float scale);
	float GetRefractionScale();

	void SetPixelShader(std::shared_ptr<SimplePixelShader> pix);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vert);

	void SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix);
	std::shared_ptr<SimplePixelShader> GetRefractivePixelShader();

	std::string GetTextureFilenameKey(PBRTextureTypes textureType);
	void SetTextureFilenameKey(PBRTextureTypes textureType, std::string newFileKey);
};

class TerrainMaterial {
public:
	TerrainMaterial(std::string name);
	TerrainMaterial(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap);
	~TerrainMaterial();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBlendMap();
	void SetBlendMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newBlendMap);

	void AddMaterial(std::shared_ptr<Material> materialToAdd);
	void SetMaterialAtID(std::shared_ptr<Material> materialToSet, int id);

	std::string GetBlendMapFilenameKey();
	void SetBlendMapFilenameKey(std::string filenameKey);

	void RemoveMaterialAtID(int id);
	std::shared_ptr<Material> GetMaterialAtID(int id);

	void RemoveMaterialByName(std::string name);
	std::shared_ptr<Material> GetMaterialByName(std::string name);

	void SetUsingBlendMap(bool usingBlendMap);
	bool GetUsingBlendMap();

	std::string GetName();
	void SetName(std::string name);

	std::shared_ptr<SimplePixelShader> GetPixelShader();
	void SetPixelShader(std::shared_ptr<SimplePixelShader> pixShader);

	std::shared_ptr<SimpleVertexShader> GetVertexShader();
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertShader);

	size_t GetMaterialCount();

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap;
	std::vector<std::shared_ptr<Material>> allMaterials;

	std::string name;
	std::string blendMapFilenameKey;

	std::shared_ptr<SimplePixelShader> pixShader;
	std::shared_ptr<SimpleVertexShader> vertShader;

	bool enabled;
	bool blendMapEnabled;
};