#include "../Headers/Texture.h"

Texture::Texture() {

}

Texture::Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
				 std::string fileKey,
				 std::string name,
				 AssetPathIndex assetPathIndex)
{
	this->coreTexture = texture;
	this->fileKey = fileKey;
	this->name = name;
	this->assetPathIndex = assetPathIndex;
}

Texture::~Texture() {

}

void Texture::SetTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture) {
	this->coreTexture = texture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture::GetTexture() {
	return this->coreTexture;
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