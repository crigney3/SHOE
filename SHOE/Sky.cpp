#include "Sky.h"


Sky::Sky(Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 
		 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture, 
		 const char* modelFile, 
		 std::map<std::string, std::shared_ptr<SimplePixelShader>>* pixShaders,
		 std::map<std::string, std::shared_ptr<SimpleVertexShader>>* vertShaders,
		 Microsoft::WRL::ComPtr<ID3D11Device> device,
		 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	this->samplerOptions = samplerOptions;
	this->skyGeometry = std::make_shared<Mesh>(modelFile, device);
	this->skyPixelShader = pixShaders->at("SkyPS");
	this->skyVertexShader = vertShaders->at("SkyVS");
	this->textureSRV = skyTexture;

	this->vertShaders = vertShaders;
	this->pixShaders = pixShaders;

	this->device = device;
	this->context = context;

	D3D11_RASTERIZER_DESC rDescription = {};
	rDescription.FillMode = D3D11_FILL_SOLID;
	rDescription.CullMode = D3D11_CULL_FRONT;

	device->CreateRasterizerState(&rDescription, &this->rasterizerOptions);

	D3D11_DEPTH_STENCIL_DESC depthDescription = {};
	depthDescription.DepthEnable = true;
	depthDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	device->CreateDepthStencilState(&depthDescription, &this->depthType);

	IBLCreateIrradianceMap();
	IBLCreateConvolvedSpecularMap();
	IBLCreateBRDFLookUpTexture();
}

Sky::Sky(Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerOptions, 
		 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyTexture, 
		 std::shared_ptr<Mesh> cubeMesh, 
		 std::map<std::string, std::shared_ptr<SimplePixelShader>>* pixShaders,
		 std::map<std::string, std::shared_ptr<SimpleVertexShader>>* vertShaders,
		 Microsoft::WRL::ComPtr<ID3D11Device> device,
		 Microsoft::WRL::ComPtr<ID3D11DeviceContext> context) {
	this->samplerOptions = samplerOptions;
	this->skyGeometry = cubeMesh;
	this->skyPixelShader = pixShaders->at("SkyPS");
	this->skyVertexShader = vertShaders->at("SkyVS");
	this->textureSRV = skyTexture;

	this->vertShaders = vertShaders;
	this->pixShaders = pixShaders;

	this->device = device;
	this->context = context;

	D3D11_RASTERIZER_DESC rDescription = {};
	rDescription.FillMode = D3D11_FILL_SOLID;
	rDescription.CullMode = D3D11_CULL_FRONT;

	device->CreateRasterizerState(&rDescription, &this->rasterizerOptions);

	D3D11_DEPTH_STENCIL_DESC depthDescription = {};
	depthDescription.DepthEnable = true;
	depthDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	device->CreateDepthStencilState(&depthDescription, &this->depthType);

	IBLCreateIrradianceMap();
	IBLCreateConvolvedSpecularMap();
	IBLCreateBRDFLookUpTexture();
}

Sky::~Sky() {

}

void Sky::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam) {
	context->RSSetState(this->rasterizerOptions.Get());
	context->OMSetDepthStencilState(this->depthType.Get(), 0);

	this->skyPixelShader->SetShader();

	this->skyPixelShader->SetSamplerState("sampleState", this->samplerOptions.Get());
	this->skyPixelShader->SetShaderResourceView("textureSky", this->textureSRV.Get());

	this->skyPixelShader->CopyAllBufferData();

	this->skyVertexShader->SetShader();

	this->skyVertexShader->SetMatrix4x4("viewMat", cam->GetViewMatrix());
	this->skyVertexShader->SetMatrix4x4("projMat", cam->GetProjectionMatrix());

	this->skyVertexShader->CopyAllBufferData();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, this->skyGeometry->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(this->skyGeometry->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);


	context->DrawIndexed(
		this->skyGeometry->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
		0,     // Offset to the first index we want to use
		0);    // Offset to add to each index when looking up vertices

	context->RSSetState(nullptr);
	context->OMSetDepthStencilState(nullptr, 0);
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

	vertShaders->at("FullscreenVS")->SetShader();
	pixShaders->at("IrradiancePS")->SetShader();
	pixShaders->at("IrradiancePS")->SetShaderResourceView("EnvironmentMap", textureSRV.Get());
	pixShaders->at("IrradiancePS")->SetSamplerState("BasicSampler", samplerOptions.Get());

	for (int face = 0; face < 6; face++) {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.ArraySize = 1;
		rtvDesc.Texture2DArray.FirstArraySlice = face;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Format = texDesc.Format;

		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
		device->CreateRenderTargetView(finalIrrMapTexture.Get(), &rtvDesc, rtv.GetAddressOf());

		float black[4] = {};
		context->ClearRenderTargetView(rtv.Get(), black);
		context->OMSetRenderTargets(1, rtv.GetAddressOf(), 0);

		pixShaders->at("IrradiancePS")->SetInt("faceIndex", face);
		pixShaders->at("IrradiancePS")->SetFloat("sampleStepPhi", 0.025f);
		pixShaders->at("IrradiancePS")->SetFloat("sampleStepTheta", 0.025f);
		pixShaders->at("IrradiancePS")->CopyAllBufferData();

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

	vertShaders->at("FullscreenVS")->SetShader();
	pixShaders->at("SpecularConvolutionPS")->SetShader();
	pixShaders->at("SpecularConvolutionPS")->SetShaderResourceView("EnvironmentMap", textureSRV.Get());
	pixShaders->at("SpecularConvolutionPS")->SetSamplerState("BasicSampler", samplerOptions.Get());

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

			pixShaders->at("SpecularConvolutionPS")->SetFloat("roughness", mipLevel / (float)(mipLevelCount - 1));
			pixShaders->at("SpecularConvolutionPS")->SetInt("faceIndex", face);
			pixShaders->at("SpecularConvolutionPS")->SetInt("mipLevel", mipLevel);
			pixShaders->at("SpecularConvolutionPS")->CopyAllBufferData();

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

	vertShaders->at("FullscreenVS")->SetShader();
	pixShaders->at("BRDFLookupTablePS")->SetShader();

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