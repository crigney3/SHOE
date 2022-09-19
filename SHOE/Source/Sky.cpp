#include "../Headers/Sky.h"

Sky::Sky(Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 
		 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture, 
		 std::vector<std::shared_ptr<SimplePixelShader>> pixShaders,
		 std::vector<std::shared_ptr<SimpleVertexShader>> vertShaders,
		 Microsoft::WRL::ComPtr<ID3D11Device> device,
		 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
		 std::string name) {
	this->samplerOptions = samplerOptions;
	this->skyPixelShader = pixShaders[0];
	this->skyVertexShader = vertShaders[0];
	this->textureSRV = skyTexture;

	this->vertShaders = vertShaders;
	this->pixShaders = pixShaders;

	this->device = device;
	this->context = context;

	this->name = name;
	this->enabled = true;

	IBLCreateIrradianceMap();
	IBLCreateConvolvedSpecularMap();
	IBLCreateBRDFLookUpTexture();
}

Sky::~Sky() {

}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetBRDFLookupTexture() {
	return this->lookupTexture;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetConvolvedSpecularCubeMap() {
	return this->convolvedSpecularCM;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetIrradianceCubeMap() {
	return this->irradianceCM;
}

int Sky::GetIBLMipLevelCount() {
	return this->mipLevelCount;
}

void Sky::IBLCreateIrradianceMap() {
	Microsoft::WRL::ComPtr<ID3D11Texture2D> finalIrrMapTexture;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = CMFaceSize;
	texDesc.Height = CMFaceSize;
	texDesc.ArraySize = 6;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	texDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&texDesc, 0, finalIrrMapTexture.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.Format = texDesc.Format;
	device->CreateShaderResourceView(finalIrrMapTexture.Get(), &srvDesc, irradianceCM.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> prevRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> prevDSV;
	context->OMGetRenderTargets(1, prevRTV.GetAddressOf(), prevDSV.GetAddressOf());

	unsigned int vpCount = 1;
	D3D11_VIEWPORT prevVP = {};
	context->RSGetViewports(&vpCount, &prevVP);

	D3D11_VIEWPORT vp = {};
	vp.Width = (float)CMFaceSize;
	vp.Height = (float)CMFaceSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	vertShaders[1]->SetShader();
	pixShaders[1]->SetShader();
	pixShaders[1]->SetShaderResourceView("EnvironmentMap", textureSRV.Get());
	pixShaders[1]->SetSamplerState("BasicSampler", samplerOptions.Get());

	for (int face = 0; face < 6; face++) {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = 1;
		rtvDesc.Texture2DArray.FirstArraySlice = face;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Format = texDesc.Format;

		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
		device->CreateRenderTargetView(finalIrrMapTexture.Get(), &rtvDesc, rtv.GetAddressOf());

		//context->Flush();

		float black[4] = {};
		context->ClearRenderTargetView(rtv.Get(), black);
		context->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);

		pixShaders[1]->SetInt("faceIndex", face);
		pixShaders[1]->SetFloat("sampleStepPhi", 0.025f);
		pixShaders[1]->SetFloat("sampleStepTheta", 0.025f);
		pixShaders[1]->CopyAllBufferData();

		context->Draw(3, 0);

		context->Flush();
	}

	context->OMSetRenderTargets(1, prevRTV.GetAddressOf(), prevDSV.Get());
	context->RSSetViewports(1, &prevVP);
}

void Sky::IBLCreateConvolvedSpecularMap() {
	Microsoft::WRL::ComPtr<ID3D11Texture2D> finalSpecMapTexture;
	mipLevelCount = max((int)(log2(CMFaceSize)) + 1 - mipLevelSkip, 1);

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = CMFaceSize;
	texDesc.Height = CMFaceSize;
	texDesc.ArraySize = 6;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.MipLevels = mipLevelCount;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	texDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&texDesc, 0, finalSpecMapTexture.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = mipLevelCount;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.Format = texDesc.Format;
	device->CreateShaderResourceView(finalSpecMapTexture.Get(), &srvDesc, convolvedSpecularCM.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> prevRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> prevDSV;
	context->OMGetRenderTargets(1, prevRTV.GetAddressOf(), prevDSV.GetAddressOf());

	unsigned int vpCount = 1;
	D3D11_VIEWPORT prevVP = {};
	context->RSGetViewports(&vpCount, &prevVP);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	vertShaders[1]->SetShader();
	pixShaders[2]->SetShader();
	pixShaders[2]->SetShaderResourceView("EnvironmentMap", textureSRV.Get());
	pixShaders[2]->SetSamplerState("BasicSampler", samplerOptions.Get());

	for (int mipLevel = 0; mipLevel < mipLevelCount; mipLevel++) {
		for (int face = 0; face < 6; face++) {
			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.ArraySize = 1;
			rtvDesc.Texture2DArray.FirstArraySlice = face;
			rtvDesc.Texture2DArray.MipSlice = mipLevel;
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
			device->CreateRenderTargetView(finalSpecMapTexture.Get(), &rtvDesc, rtv.GetAddressOf());

			float black[4] = {};
			context->ClearRenderTargetView(rtv.Get(), black);
			context->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);

			D3D11_VIEWPORT vp = {};
			vp.Width = (float)pow(2, mipLevelCount + mipLevelSkip - 1 - mipLevel);
			vp.Height = vp.Width;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			context->RSSetViewports(1, &vp);

			pixShaders[2]->SetFloat("roughness", mipLevel / (float)(mipLevelCount - 1));
			pixShaders[2]->SetInt("faceIndex", face);
			pixShaders[2]->SetInt("mipLevel", mipLevel);
			pixShaders[2]->CopyAllBufferData();

			context->Draw(3, 0);

			context->Flush();
		}
	}

	context->OMSetRenderTargets(1, prevRTV.GetAddressOf(), prevDSV.Get());
	context->RSSetViewports(1, &prevVP);
}

void Sky::IBLCreateBRDFLookUpTexture() {
	Microsoft::WRL::ComPtr<ID3D11Texture2D> finalBRDFMapTexture;

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = lookupTextureSize;
	texDesc.Height = lookupTextureSize;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R16G16_UNORM;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&texDesc, 0, finalBRDFMapTexture.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Format = texDesc.Format;
	device->CreateShaderResourceView(finalBRDFMapTexture.Get(), &srvDesc, lookupTexture.GetAddressOf());

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> prevRTV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> prevDSV;
	context->OMGetRenderTargets(1, prevRTV.GetAddressOf(), prevDSV.GetAddressOf());

	unsigned int vpCount = 1;
	D3D11_VIEWPORT prevVP = {};
	context->RSGetViewports(&vpCount, &prevVP);

	D3D11_VIEWPORT vp = {};
	vp.Width = (float)lookupTextureSize;
	vp.Height = (float)lookupTextureSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	vertShaders[1]->SetShader();
	pixShaders[3]->SetShader();

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Format = texDesc.Format;

	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
	device->CreateRenderTargetView(finalBRDFMapTexture.Get(), &rtvDesc, rtv.GetAddressOf());

	float black[4] = {};
	context->ClearRenderTargetView(rtv.Get(), black);
	context->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);

	context->Draw(3, 0);

	context->Flush();

	context->OMSetRenderTargets(1, prevRTV.GetAddressOf(), prevDSV.Get());
	context->RSSetViewports(1, &prevVP);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Sky::GetSkyTexture() {
	return this->textureSRV;
}

std::shared_ptr<SimplePixelShader> Sky::GetPixShader() {
	return this->skyPixelShader;
}

std::shared_ptr<SimpleVertexShader> Sky::GetVertShader() {
	return this->skyVertexShader;
}

Microsoft::WRL::ComPtr<ID3D11SamplerState> Sky::GetSampler()
{
	return samplerOptions;
}

void Sky::SetEnabled(bool value) {
	this->enabled = value;
}

bool Sky::IsEnabled() {
	return this->enabled;
}

std::string Sky::GetName() {
	return this->name;
}

void Sky::SetName(std::string name) {
	this->name = name;
}

bool Sky::GetFilenameKeyType() {
	return this->filenameKeyType;
}

void Sky::SetFilenameKeyType(bool FKType) {
	this->filenameKeyType = FKType;
}

std::string Sky::GetFilenameKey() {
	return this->filenameKey;
}

void Sky::SetFilenameKey(std::string filenameKey) {
	this->filenameKey = filenameKey;
}

std::string Sky::GetFileExtension() {
	return this->fileExtension;
}

void Sky::SetFileExtension(std::string fileExtension) {
	this->fileExtension = fileExtension;
}