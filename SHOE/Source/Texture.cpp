#include "../Headers/Texture.h"

#pragma region Texture

Texture::Texture() {

}

<<<<<<< HEAD
Texture::Texture(std::string fileKey,
=======
Texture::Texture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
				 std::string fileKey,
>>>>>>> 6be2a01f49bf99da24916a08a5518ac0493f2b05
				 std::string name,
				 AssetPathIndex assetPathIndex)
{
	this->fileKey = fileKey;
	this->name = name;
	this->assetPathIndex = assetPathIndex;
<<<<<<< HEAD
	this->tempTexture = false;
=======
>>>>>>> 6be2a01f49bf99da24916a08a5518ac0493f2b05
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
<<<<<<< HEAD
}

bool Texture::IsTextureTemp() {
	return this->tempTexture;
}

void Texture::SetIsTextureTemp(bool tempState) {
	this->tempTexture = tempState;
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

D3D11_TEXTURE2D_DESC DX11Texture::GetTextureDesc() {
	return this->textureDesc;
}

void DX11Texture::SetTextureDesc(D3D11_TEXTURE2D_DESC newDesc) {
	this->textureDesc = newDesc;
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> DX11Texture::GetInternalTexture() {
	return this->internalTexture;
}

void DX11Texture::SetInternalTexture(Microsoft::WRL::ComPtr<ID3D11Texture2D> newInternalTexture) {
	this->internalTexture = newInternalTexture;
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
=======
}
>>>>>>> 6be2a01f49bf99da24916a08a5518ac0493f2b05
