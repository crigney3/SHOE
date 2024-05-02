#include "../Headers/Material.h"
#include "..\Headers\AssetManager.h"
#include "../Headers/ComponentManager.h"

#pragma region Material

Material::Material(std::shared_ptr<SimplePixelShader> pix, 
				   std::shared_ptr<SimpleVertexShader> vert, 
				   std::string name,
				   bool transparent,
				   bool refractive) {
	this->colorTint = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	this->pixShader = pix;
	this->vertShader = vert;
	this->uvTiling = 1.0f;
	this->enabled = true;
	this->name = name;
	this->transparent = transparent;
	this->refractive = refractive;

	// For now, these are only set by the Setter functions
	this->indexOfRefraction = 0.5f;
	this->refractionScale = 0.1f;
	this->refractivePixShader = NULL;
}

Material::~Material() {

}

std::string Material::GetName() {
	return this->name;
}

void Material::SetName(std::string name) {
	this->name = name;
}

DirectX::XMFLOAT4 Material::GetTint() {
	return this->colorTint;
}

void Material::SetTint(DirectX::XMFLOAT4 tint) {
	this->colorTint = tint;
}

std::shared_ptr<SimplePixelShader> Material::GetPixShader() {
	return this->pixShader;
}

std::shared_ptr<SimpleVertexShader> Material::GetVertShader() {
	return this->vertShader;
}

float Material::GetTiling() {
	return this->uvTiling;
}

void Material::SetTiling(float uv) {
	this->uvTiling = uv;
}

std::string Material::GetTextureFilenameKey(PBRTextureTypes textureType) {
	switch (textureType) {
		case PBRTextureTypes::ALBEDO:
			return this->albedoFileKey;
			break;
		case PBRTextureTypes::NORMAL:
			return this->normalsFileKey;
			break;
		case PBRTextureTypes::METAL:
			return this->metalFileKey;
			break;
		case PBRTextureTypes::ROUGH:
			return this->roughnessFileKey;
			break;
		default:
			return "";
			break;
	}
}

void Material::SetTextureFilenameKey(PBRTextureTypes textureType, std::string newFileKey) {
	switch (textureType) {
		case PBRTextureTypes::ALBEDO:
			this->albedoFileKey = newFileKey;
			break;
		case PBRTextureTypes::NORMAL:
			this->normalsFileKey = newFileKey;
			break;
		case PBRTextureTypes::METAL:
			this->metalFileKey = newFileKey;
			break;
		case PBRTextureTypes::ROUGH:
			this->roughnessFileKey = newFileKey;
			break;
		default:
			break;
	}
}

/// <summary>
/// Sets whether to render the attached textures with transparency
/// </summary>
/// <param name="transparent">Texture has transparency</param>
void Material::SetTransparent(bool transparent) {
	if (this->transparent != transparent) {
		// If this is turning off transparency, disable refraction as well
		if (!transparent) this->refractive = false;
		this->transparent = transparent;
		ComponentManager::Sort<MeshRenderer>();
	}
}

bool Material::GetTransparent() {
	return this->transparent;
}

void Material::SetRefractive(bool refractive) {
	this->refractive = refractive;

	// If this material is refractive, we can assume it should
	// also be transparent.
	if(refractive) this->transparent = true;
}

bool Material::GetRefractive() {
	return this->refractive;
}

void Material::SetIndexOfRefraction(float index) {
	this->indexOfRefraction = index;
}

float Material::GetIndexOfRefraction() {
	return this->indexOfRefraction;
}

void Material::SetRefractionScale(float scale) {
	this->refractionScale = scale;
}

float Material::GetRefractionScale() {
	return this->refractionScale;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetSamplerState() {
	printf("Error: Calling virtual base class!");
	Microsoft::WRL::ComPtr<ID3D11SamplerState> error;
	return error;
}

void Material::SetSamplerState(void*) {
	printf("Error: Calling virtual base class!");
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetClampSamplerState() {
	printf("Error: Calling virtual base class!");
	Microsoft::WRL::ComPtr<ID3D11SamplerState> error;
	return error;
}

void Material::SetClampSamplerState(void*) {
	printf("Error: Calling virtual base class!");
}

std::shared_ptr<Texture> Material::GetNormalMap() {
	printf("Error: Calling virtual base class!");
	std::shared_ptr<Texture> error;
	return error;
}

std::shared_ptr<Texture> Material::GetMetalMap() {
	printf("Error: Calling virtual base class!");
	std::shared_ptr<Texture> error;
	return error;
}

std::shared_ptr<Texture> Material::GetRoughMap() {
	printf("Error: Calling virtual base class!");
	std::shared_ptr<Texture> error;
	return error;
}

std::shared_ptr<Texture> Material::GetTexture() {
	printf("Error: Calling virtual base class!");
	std::shared_ptr<Texture> error;
	return error;
}

void Material::SetNormalMap(std::shared_ptr<Texture> normals) {
	printf("Error: Calling virtual base class!");
	return;
}

void Material::SetMetalMap(std::shared_ptr<Texture> roughMap) {
	printf("Error: Calling virtual base class!");
	return;
}

void Material::SetRoughMap(std::shared_ptr<Texture> metalMap) {
	printf("Error: Calling virtual base class!");
	return;
}

void Material::SetTexture(std::shared_ptr<Texture> texture) {
	printf("Error: Calling virtual base class!");
	return;
}

void Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pix) {
	this->pixShader = pix;
	ComponentManager::Sort<MeshRenderer>();
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vert) {
	this->vertShader = vert;
	ComponentManager::Sort<MeshRenderer>();
}

void Material::SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix) {
	this->refractivePixShader = refractPix;
}

std::shared_ptr<SimplePixelShader> Material::GetRefractivePixelShader() {
	return this->refractivePixShader;
}

#pragma endregion

#pragma region DX11Material

// Deprecated
//DX11Material::DX11Material(std::shared_ptr<SimplePixelShader> pix,
//						   std::shared_ptr<SimpleVertexShader> vert,
//						   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
//						   Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
//						   Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
//						   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap,
//						   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap,
//						   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap,
//						   std::string name,
//						   bool transparent,
//						   bool refractive)
//	: Material(pix, vert, name, transparent, refractive)
//{
//	std::shared_ptr<DX11Texture> tex = std::make_shared<DX11Texture>(texture, )
//}

DX11Material::DX11Material(std::shared_ptr<SimplePixelShader> pix,
						   std::shared_ptr<SimpleVertexShader> vert,
						   std::shared_ptr<Texture> texture,
						   Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
						   Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
						   std::shared_ptr<Texture> normalMap,
						   std::shared_ptr<Texture> roughMap,
						   std::shared_ptr<Texture> metalMap,
						   std::string name,
						   bool transparent,
						   bool refractive) 
	: Material(pix, vert, name, transparent, refractive)
{
	this->texture = texture;
	this->textureState = textureState;
	this->normalMap = normalMap;
	this->roughMap = roughMap;
	this->metalMap = metalMap;
	this->clampState = clampState;
}

DX11Material::~DX11Material() {

}

std::shared_ptr<Texture> DX11Material::GetNormalMap() {
	return this->normalMap;
}

std::shared_ptr<Texture> DX11Material::GetMetalMap() {
	return this->metalMap;
}

std::shared_ptr<Texture> DX11Material::GetRoughMap() {
	return this->roughMap;
}

std::shared_ptr<Texture> DX11Material::GetTexture() {
	return this->texture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DX11Material::GetNormalMapSRV() {
	return this->normalMap.get()->GetDX11Texture();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DX11Material::GetMetalMapSRV() {
	return this->metalMap.get()->GetDX11Texture();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DX11Material::GetRoughMapSRV() {
	return this->roughMap.get()->GetDX11Texture();
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DX11Material::GetAlbedoMapSRV() {
	return this->texture.get()->GetDX11Texture();
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> DX11Material::GetSamplerState() {
	return this->textureState;
}

void DX11Material::SetSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> texSamplerState) {
	this->textureState = texSamplerState;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> DX11Material::GetClampSamplerState() {
	return this->clampState;
}

void DX11Material::SetClampSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSamplerState) {
	this->clampState = clampSamplerState;
}

#pragma endregion

#pragma region DX12Material

DX12Material::DX12Material(std::shared_ptr<SimplePixelShader> pix,
						   std::shared_ptr<SimpleVertexShader> vert,
						   std::string name,
						   bool transparent,
						   bool refractive) 
	: Material(pix, vert, name, transparent, refractive)
{

}

DX12Material::DX12Material(std::shared_ptr<SimplePixelShader> pix,
						   std::shared_ptr<SimpleVertexShader> vert,
						   std::shared_ptr<RootSignature> rootSignature,
						   Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeState,
						   std::string name,
						   bool transparent,
						   bool refractive) 
	: Material(pix, vert, name, transparent, refractive)
{
	this->rootSig = rootSignature;
	this->pipelineState = pipeState;
}

DX12Material::~DX12Material() {

}

std::shared_ptr<RootSignature> DX12Material::GetRootSignature() {
	return this->rootSig;
}

void DX12Material::SetRootSignature(std::shared_ptr<RootSignature> rootSignature) {
	this->rootSig = rootSignature;
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> DX12Material::GetPipelineState() {
	return this->pipelineState;
}

void DX12Material::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeState) {
	this->pipelineState = pipeState;
}

void DX12Material::SetPixelShader(std::shared_ptr<SimplePixelShader> pix) {
	this->pixShader = pix;
	ComponentManager::Sort<MeshRenderer>();
}

void DX12Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vert) {
	this->vertShader = vert;
	ComponentManager::Sort<MeshRenderer>();
}

void DX12Material::SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix) {
	this->refractivePixShader = refractPix;
}

std::shared_ptr<SimplePixelShader> DX12Material::GetRefractivePixelShader() {
	return this->refractivePixShader;
}

#pragma endregion

#pragma region TerrainMaterial

TerrainMaterial::TerrainMaterial(std::string name) {
	this->allMaterials = std::vector<std::shared_ptr<Material>>();
	this->name = name;

	this->enabled = true;
}

TerrainMaterial::~TerrainMaterial() {
	this->allMaterials.clear();
}

void TerrainMaterial::SetUsingBlendMap(bool usingBlendMap) {
	this->blendMapEnabled = usingBlendMap;
}

bool TerrainMaterial::GetUsingBlendMap() {
	return this->blendMapEnabled;
}

void TerrainMaterial::SetBlendMapFilenameKey(std::string filenameKey) {
	this->blendMapFilenameKey = filenameKey;
}

std::string TerrainMaterial::GetBlendMapFilenameKey() {
	return this->blendMapFilenameKey;
}

std::string TerrainMaterial::GetName() {
	return this->name;
}

void TerrainMaterial::SetName(std::string name) {
	this->name = name;
}

void TerrainMaterial::AddMaterial(std::shared_ptr<Material> materialToAdd) {
	this->allMaterials.push_back(materialToAdd);
}

void TerrainMaterial::SetMaterialAtID(std::shared_ptr<Material> materialToSet, int id) {
	this->allMaterials[id] = materialToSet;
}

void TerrainMaterial::RemoveMaterialAtID(int id) {
	this->allMaterials[id] = nullptr;
}

std::shared_ptr<Material> TerrainMaterial::GetMaterialAtID(int id) {
	return this->allMaterials[id];
}

void TerrainMaterial::RemoveMaterialByName(std::string name) {
	for (auto tm : allMaterials) {
		if (tm->GetName() == name) {
			tm.reset();
			return;
		}
	}
}

std::shared_ptr<Material> TerrainMaterial::GetMaterialByName(std::string name) {
	for (auto tm : allMaterials) {
		if (tm->GetName() == name) {
			return tm;
		}
	}
}

size_t TerrainMaterial::GetMaterialCount() {
	return this->allMaterials.size();
}

std::shared_ptr<SimplePixelShader> TerrainMaterial::GetPixelShader() {
	return this->pixShader;
}

void TerrainMaterial::SetPixelShader(std::shared_ptr<SimplePixelShader> pixShader) {
	this->pixShader = pixShader;
}

std::shared_ptr<SimpleVertexShader> TerrainMaterial::GetVertexShader() {
	return this->vertShader;
}

void TerrainMaterial::SetVertexShader(std::shared_ptr<SimpleVertexShader> vertShader) {
	this->vertShader = vertShader;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TerrainMaterial::GetBlendMap() {
	printf("Error: Calling virtual base class!");
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> error;
	return error;
}

void TerrainMaterial::SetBlendMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newBlendMap) {
	printf("Error: Calling virtual base class!");
}

#pragma endregion

#pragma region DX11TerrainMaterial

DX11TerrainMaterial::DX11TerrainMaterial(std::string name, 
										 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap) 
	: TerrainMaterial(name)
{
	this->blendMap = blendMap;
	this->blendMapEnabled = true;
}

DX11TerrainMaterial::~DX11TerrainMaterial() {

}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DX11TerrainMaterial::GetBlendMap() {
	return this->blendMap;
}

void DX11TerrainMaterial::SetBlendMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newBlendMap) {
	this->blendMap = newBlendMap;
	this->blendMapEnabled = true;
}

#pragma endregion

#pragma region DX12TerrainMaterial



#pragma endregion