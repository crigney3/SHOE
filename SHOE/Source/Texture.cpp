#include "../Headers/Texture.h"

#pragma region Texture

Texture::Texture() {

}

Texture::Texture(std::string fileKey,
				 std::string name,
				 AssetPathIndex assetPathIndex)
{
	this->fileKey = fileKey;
	this->name = name;
	this->assetPathIndex = assetPathIndex;
}

Texture::~Texture() {

}

void Texture::SetTexture() {
	printf("Error: Calling virtual base class!");
	return;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture::GetDX11Texture() {
	return this->coreDx11Texture;
}

std::string Texture::GetName() {
	return this->name;
}

void Texture::SetName(std::string name){
	this->name = name;
}

std::string Texture::GetTextureFilenameKey(){
	return this->fileKey;
}

void Texture::SetTextureFilenameKey(std::string newFileKey){
	this->fileKey = newFileKey;
}

AssetPathIndex Texture::GetAssetPathIndex() {
	return this->assetPathIndex;
}

void Texture::SetAssetPathIndex(AssetPathIndex pathIndex) {
	this->assetPathIndex = pathIndex;
}

#pragma endregion

#pragma region DX11Texture

DX11Texture::DX11Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture, 
						 std::string fileKey,
						 std::string name,
						 AssetPathIndex assetPathIndex)
	: Texture(fileKey, name, assetPathIndex)
{
	this->coreDx11Texture = coreTexture;
	this->fileKey = fileKey;
	this->name = name;
	this->assetPathIndex = assetPathIndex;
}

DX11Texture::~DX11Texture() {

}

void DX11Texture::SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture) {
	this->coreDx11Texture = texture;
}

#pragma endregion

#pragma region DX12Texture

// TODO: change types, make this actually work
DX12Texture::DX12Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coreTexture,
						 std::string fileKey,
						 std::string name,
						 AssetPathIndex assetPathIndex){

}

DX12Texture::~DX12Texture() {

}

void DX12Texture::SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture) {

}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> DX12Texture::GetTexture() {
	return nullptr;
}

#pragma endregion