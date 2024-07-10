#pragma once

#include "SimpleShader.h"
#include "RootSignature.h"
#include <memory>
#include "Texture.h"
#include <vector>

enum PBRTextureTypes {
	ALBEDO,
	NORMAL,
	METAL,
	ROUGH
};

//enum MaterialTypes {
//	BaseMaterial,
//	DX11Material,
//	DX12Material,
//	DX11TerrainMaterial,
//	DX12TerrainMaterial
//};

class Material
{
protected:
	float uvTiling;
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimplePixelShader> pixShader;
	std::shared_ptr<SimplePixelShader> refractivePixShader;
	std::shared_ptr<SimpleVertexShader> vertShader;

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
	Material(std::shared_ptr<SimplePixelShader> pix,
			 std::shared_ptr<SimpleVertexShader> vert,
			 std::string name = "material",
			 bool transparent = false,
			 bool refractive = false);
	~Material();

	DirectX::XMFLOAT4 GetTint();
	void SetTint(DirectX::XMFLOAT4 tint);
	std::shared_ptr<SimplePixelShader> GetPixShader();
	std::shared_ptr<SimpleVertexShader> GetVertShader();

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

	virtual Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSamplerState();
	virtual void SetSamplerState(void*);

	virtual Microsoft::WRL::ComPtr<ID3D11SamplerState> GetClampSamplerState();
	virtual void SetClampSamplerState(void*);

	virtual std::shared_ptr<Texture> GetNormalMap();
	virtual std::shared_ptr<Texture> GetMetalMap();
	virtual std::shared_ptr<Texture> GetRoughMap();
	virtual std::shared_ptr<Texture> GetTexture();

	virtual void SetTexture(std::shared_ptr<Texture> texture);
	virtual void SetNormalMap(std::shared_ptr<Texture> normals);
	virtual void SetRoughMap(std::shared_ptr<Texture> roughMap);
	virtual void SetMetalMap(std::shared_ptr<Texture> metalMap);

	virtual void SetPixelShader(std::shared_ptr<SimplePixelShader> pix);
	virtual void SetVertexShader(std::shared_ptr<SimpleVertexShader> vert);

	virtual void SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix);
	virtual std::shared_ptr<SimplePixelShader> GetRefractivePixelShader();

	std::string GetTextureFilenameKey(PBRTextureTypes textureType);
	void SetTextureFilenameKey(PBRTextureTypes textureType, std::string newFileKey);
};

class DX11Material : public Material {

public:

	// Deprecated
	//DX11Material(std::shared_ptr<SimplePixelShader> pix,
	//			 std::shared_ptr<SimpleVertexShader> vert,
	//			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
	//			 Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
	//			 Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
	//			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap,
	//			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap,
	//			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap,
	//			 std::string name = "material",
	//			 bool transparent = false,
	//			 bool refractive = false);
	DX11Material(std::shared_ptr<SimplePixelShader> pix,
				 std::shared_ptr<SimpleVertexShader> vert,
				 std::shared_ptr<Texture> texture,
				 Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
				 Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
				 std::shared_ptr<Texture> normalMap,
				 std::shared_ptr<Texture> roughMap,
				 std::shared_ptr<Texture> metalMap,
				 std::string name = "material",
				 bool transparent = false,
				 bool refractive = false);
	~DX11Material();

	std::shared_ptr<Texture> GetNormalMap();
	std::shared_ptr<Texture> GetMetalMap();
	std::shared_ptr<Texture> GetRoughMap();
	std::shared_ptr<Texture> GetTexture();

	void SetTexture(std::shared_ptr<Texture> texture);
	void SetNormalMap(std::shared_ptr<Texture> normals);
	void SetRoughMap(std::shared_ptr<Texture> roughMap);
	void SetMetalMap(std::shared_ptr<Texture> metalMap);

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetNormalMapSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetMetalMapSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRoughMapSRV();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetAlbedoMapSRV();

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSamplerState();
	void SetSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> texSamplerState);

	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetClampSamplerState();
	void SetClampSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSamplerState);

protected:

	std::shared_ptr<Texture> texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;
	std::shared_ptr<Texture> normalMap;
	std::shared_ptr<Texture> metalMap;
	std::shared_ptr<Texture> roughMap;
};

class DX12Material : public Material {
public:

	DX12Material(std::shared_ptr<SimplePixelShader> pix,
				 std::shared_ptr<SimpleVertexShader> vert,
				 std::string name = "material",
				 bool transparent = false,
				 bool refractive = false);
	DX12Material(std::shared_ptr<SimplePixelShader> pix,
				 std::shared_ptr<SimpleVertexShader> vert,
				 std::shared_ptr<RootSignature> rootSignature,
				 Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeState,
				 std::string name = "material",
				 bool transparent = false,
				 bool refractive = false);
	~DX12Material();

	std::shared_ptr<RootSignature> GetRootSignature();
	void SetRootSignature(std::shared_ptr<RootSignature> rootSignature);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeState);

	void SetPixelShader(std::shared_ptr<SimplePixelShader> pix);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vert);

	void SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix);
	std::shared_ptr<SimplePixelShader> GetRefractivePixelShader();

protected:

	// The root signature and pipeline state store all of the
	// textures and other SRV info.
	std::shared_ptr<RootSignature> rootSig;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

};

class TerrainMaterial {
public:
	TerrainMaterial(std::string name);
	~TerrainMaterial();

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

	virtual std::shared_ptr<SimplePixelShader> GetPixelShader();
	virtual void SetPixelShader(std::shared_ptr<SimplePixelShader> pixShader);

	virtual std::shared_ptr<SimpleVertexShader> GetVertexShader();
	virtual void SetVertexShader(std::shared_ptr<SimpleVertexShader> vertShader);

	virtual Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBlendMap();
	virtual void SetBlendMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newBlendMap);

	virtual std::shared_ptr<Texture> GetBlendMapTexture();
	virtual void SetBlendMapFromTexture(std::shared_ptr<Texture> newBlendMap);

	size_t GetMaterialCount();

protected:
	std::vector<std::shared_ptr<Material>> allMaterials;

	std::string name;
	std::string blendMapFilenameKey;

	std::shared_ptr<SimplePixelShader> pixShader;
	std::shared_ptr<SimpleVertexShader> vertShader;

	bool enabled;
	bool blendMapEnabled = false;
};

class DX11TerrainMaterial : public TerrainMaterial {
public:

	DX11TerrainMaterial(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap);
	DX11TerrainMaterial(std::string name, std::shared_ptr<Texture> blendMap);
	~DX11TerrainMaterial();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetBlendMap();
	void SetBlendMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newBlendMap);

	std::shared_ptr<Texture> GetBlendMapTexture();
	void SetBlendMapFromTexture(std::shared_ptr<Texture> newBlendMap);

protected:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap;
	std::shared_ptr<Texture> blendMapTexture;
};

class DX12TerrainMaterial : public TerrainMaterial {

};