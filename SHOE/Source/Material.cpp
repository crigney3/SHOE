#include "../Headers/Material.h"

Material::Material(DirectX::XMFLOAT4 tint, 
				   std::shared_ptr<SimplePixelShader> pix, 
				   std::shared_ptr<SimpleVertexShader> vert, 
				   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
				   Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState, 
				   Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState, 
				   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap, 
				   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap, 
				   Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap,
				   std::string name,
				   bool transparent,
				   bool refractive) {
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

/*float Material::GetSpecExponent() {
	return this->specularExponent;
}*/

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetNormalMap() {
	return this->normalMap;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetMetalMap() {
	return this->metalMap;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetRoughMap() {
	return this->roughMap;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Material::GetTexture() {
	return this->texture;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetSamplerState() {
	return this->textureState;
}

float Material::GetTiling() {
	return this->uvTiling;
}

void Material::SetTiling(float uv) {
	this->uvTiling = uv;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Material::GetClampSamplerState() {
	return this->clampState;
}

void Material::SetTransparent(bool transparent) {
	// If this is turning off transparency, disable refraction as well
	if (!transparent) this->refractive = false;
	this->transparent = transparent;
}

bool Material::GetTransparent() {
	return this->transparent;
}

void Material::SetRefractive(bool refractive) {
	this->refractive = refractive;

	// If this material is refractive, we can assume it should
	// also be transparent.
	this->transparent = true;
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
}

void Material::SetVertexShader(std::shared_ptr<SimpleVertexShader> vert) {
	this->vertShader = vert;
}

void Material::SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix) {
	this->refractivePixShader = refractPix;
}

std::shared_ptr<SimplePixelShader> Material::GetRefractivePixelShader() {
	return this->refractivePixShader;
}

void Material::SetEnableDisable(bool value) {
	this->enabled = value;
}

bool Material::GetEnableDisable() {
	return this->enabled;
}