#include "../Headers/Material.h"
#include "..\Headers\AssetManager.h"
#include "../Headers/ComponentManager.h"

#pragma region Material

Material::Material(DirectX::XMFLOAT4 tint,
	std::shared_ptr<SimplePixelShader> pix,
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
{
	this->colorTint = tint;
	this->pixShader = pix;
	this->vertShader = vert;
	this->texture = texture;
	this->textureState = textureState;
	this->normalMap = normalMap;
	this->roughMap = roughMap;
	this->metalMap = metalMap;
	this->uvTiling = 1.0f;
	this->clampState = clampState;
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

std::shared_ptr<Texture> Material::GetNormalMap() {
	return this->normalMap;
}

std::shared_ptr<Texture> Material::GetMetalMap() {
	return this->metalMap;
}

std::shared_ptr<Texture> Material::GetRoughMap() {
	return this->roughMap;
}

std::shared_ptr<Texture> Material::GetTexture() {
	return this->texture;
}

void Material::SetTexture(std::shared_ptr<Texture> texture) {
	this->texture = texture;
}

void Material::SetNormalMap(std::shared_ptr<Texture> normals) {
	this->normalMap = normals;
}

void Material::SetRoughMap(std::shared_ptr<Texture> roughMap) {
	this->roughMap = roughMap;
}

void Material::SetMetalMap(std::shared_ptr<Texture> metalMap) {
	this->metalMap = metalMap;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetSamplerState() {
	return this->textureState;
}

void Material::SetSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> texSamplerState) {
	this->textureState = texSamplerState;
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

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetClampSamplerState() {
	return this->clampState;
}

void Material::SetClampSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSamplerState) {
	this->clampState = clampSamplerState;
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

#pragma region TerrainMaterial

TerrainMaterial::TerrainMaterial(std::string name) {
	this->blendMap = nullptr;
	this->allMaterials = std::vector<std::shared_ptr<Material>>();
	this->name = name;

	this->enabled = true;
	this->blendMapEnabled = false;
}

TerrainMaterial::TerrainMaterial(std::string name, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap) {
	this->blendMap = blendMap;
	this->allMaterials = std::vector<std::shared_ptr<Material>>();
	this->name = name;

	this->enabled = true;
	this->blendMapEnabled = true;
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

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> TerrainMaterial::GetBlendMap() {
	return this->blendMap;
}

void TerrainMaterial::SetBlendMap(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> newBlendMap) {
	this->blendMap = newBlendMap;
	this->blendMapEnabled = true;
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

#pragma endregion