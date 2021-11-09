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
				   std::string name) {
	this->colorTint = tint;
	this->pixShader = pix;
	this->vertShader = vert;
	//this->specularExponent = shininess;
	this->texture = texture;
	this->textureState = textureState;
	this->normalMap = normalMap;
	this->roughMap = roughMap;
	this->metalMap = metalMap;
	this->uvTiling = 1.0f;
	this->clampState = clampState;
	this->enabled = true;
	this->name = name;
}

Material::~Material() {

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

void Material::SetEnableDisable(bool value) {
	this->enabled = value;
}

bool Material::GetEnableDisable() {
	return this->enabled;
}