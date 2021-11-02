#include "Renderer.h"

using namespace DirectX;

Renderer::Renderer(
        unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D11Device> device,
        Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
        Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV) 
{
	this->currentSky = globalAssets.currentSky;
    this->windowHeight = windowHeight;
    this->windowWidth = windowWidth;
    this->device = device;
    this->context = context;
    this->swapChain = swapChain;
    this->backBufferRTV = backBufferRTV;
    this->depthBufferDSV = depthBufferDSV;
	this->ambientColor = DirectX::XMFLOAT3(0.05f, 0.05f, 0.1f);
	this->mainCamera = globalAssets.globalCameras.at("mainCamera");
	this->mainShadowCamera = globalAssets.globalCameras.at("mainShadowCamera");
	this->flashShadowCamera = globalAssets.globalCameras.at("flashShadowCamera");

	InitRenderTargetViews();
}

Renderer::~Renderer(){

}

void Renderer::InitRenderTargetViews() {
	for (int i = 0; i < 4; i++) {
		D3D11_TEXTURE2D_DESC basicTexDesc = {};
		basicTexDesc.Width = windowWidth;
		basicTexDesc.Height = windowHeight;
		basicTexDesc.ArraySize = 1;
		basicTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		basicTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		basicTexDesc.MipLevels = 1;
		basicTexDesc.MiscFlags = 0;
		basicTexDesc.SampleDesc.Count = 1;
		device->CreateTexture2D(&basicTexDesc, 0, &ssaoTexture2D[i]);

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Format = basicTexDesc.Format;
		device->CreateRenderTargetView(ssaoTexture2D[i].Get(), &rtvDesc, ssaoRTVs[i].GetAddressOf());

		device->CreateShaderResourceView(ssaoTexture2D[i].Get(), 0, ssaoSRV[i].GetAddressOf());
	}

	// SSAO needs a random 4x4 texture to work
	const int textureSize = 4;
	const int totalPixels = textureSize * textureSize;
	XMFLOAT4 randomPixels[totalPixels] = {};
	for (int i = 0; i < totalPixels; i++) {
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), 0, 0);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randomVec));
	}

	// Need to pass this texture to GPU
	D3D11_TEXTURE2D_DESC basicTexDesc = {};
	basicTexDesc.Width = windowWidth;
	basicTexDesc.Height = windowHeight;
	basicTexDesc.ArraySize = 1;
	basicTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	basicTexDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	basicTexDesc.MipLevels = 1;
	basicTexDesc.MiscFlags = 0;
	basicTexDesc.SampleDesc.Count = 1;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(float) * 4 * windowWidth;

	device->CreateTexture2D(&basicTexDesc, &data, &ssaoRandomTex);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = basicTexDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(ssaoRandomTex.Get(), &srvDesc, ssaoRandomSRV.GetAddressOf());

	// Create SSAO Offset Vectors
	for (int i = 0; i < 64; i++) {
		ssaoOffsets[i] = XMFLOAT4(
			(float)RandomRange(-1, 1),
			(float)RandomRange(-1, 1),
			(float)RandomRange(0, 1),
			0
		);
		XMVECTOR offset = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		float scale = (float)i / 64;
		XMVECTOR acceleratedScale = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1),
			XMVectorSet(1, 1, 1, 1),
			scale * scale
		);
		XMStoreFloat4(&ssaoOffsets[i], offset * acceleratedScale);
	}
}

void Renderer::InitShadows() {
    //Set up buffers and data for shadows
	shadowSize = 2048;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDepthView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> envShadowDepthView;

	this->VSShadow = globalAssets.vertexShaders.at("ShadowVS");

	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowSize;
	shadowDesc.Height = shadowSize;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D* shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, &shadowTexture);

	ID3D11Texture2D* envShadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, &envShadowTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTexture, &shadowDSDesc, shadowDepthView.GetAddressOf());

	device->CreateDepthStencilView(envShadowTexture, &shadowDSDesc, envShadowDepthView.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture, &srvDesc, shadowSRV.GetAddressOf());

	device->CreateShaderResourceView(envShadowTexture, &srvDesc, envShadowSRV.GetAddressOf());

	shadowTexture->Release();
	envShadowTexture->Release();

	D3D11_SAMPLER_DESC shadowSampDesc = {};
	shadowSampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; // This is the important change from regular Sampler Desc
	shadowSampDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	shadowSampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSampDesc.BorderColor[0] = 1.0f;
	shadowSampDesc.BorderColor[1] = 1.0f;
	shadowSampDesc.BorderColor[2] = 1.0f;
	shadowSampDesc.BorderColor[3] = 1.0f;
	device->CreateSamplerState(&shadowSampDesc, &shadowSampler);

	D3D11_RASTERIZER_DESC shadowRastDesc = {};
	shadowRastDesc.FillMode = D3D11_FILL_SOLID;
	shadowRastDesc.CullMode = D3D11_CULL_BACK;
	shadowRastDesc.DepthClipEnable = true;
	shadowRastDesc.DepthBias = 100; 
	shadowRastDesc.DepthBiasClamp = 0.0f;
	shadowRastDesc.SlopeScaledDepthBias = 10.0f;
	device->CreateRasterizerState(&shadowRastDesc, &shadowRasterizer);

	shadowDepthBuffers.push_back(shadowDepthView);
	shadowDepthBuffers.push_back(envShadowDepthView);
}

void Renderer::PostResize(unsigned int windowHeight,
        unsigned int windowWidth,
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV
    ) 
{
	this->windowHeight = windowHeight;
	this->windowWidth = windowWidth;
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
}

// ------------------------------------------------------------------------
// Draws the point lights as solid color spheres - credit to Chris Cascioli
// ------------------------------------------------------------------------
void Renderer::DrawPointLights()
{
	std::shared_ptr<SimpleVertexShader> lightVS = globalAssets.vertexShaders["BasicVS"];
	std::shared_ptr<SimplePixelShader> lightPS = globalAssets.pixelShaders["SolidColorPS"];

	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", globalAssets.globalCameras["mainCamera"]->GetViewMatrix());
	lightVS->SetMatrix4x4("projection", globalAssets.globalCameras["mainCamera"]->GetProjectionMatrix());

	for (int i = 0; i < globalAssets.globalLights.size(); i++)
	{
		Light light = globalAssets.globalLights[i];

		// Only drawing points, so skip others
		if (light.type != 1.0f || light.enabled != true)
			continue;

		// Calc quick scale based on range
		// (assuming range is between 5 - 10)
		float scale = light.range / 10.0f;

		// Make the transform for this light
		DirectX::XMMATRIX rotMat = DirectX::XMMatrixIdentity();
		DirectX::XMMATRIX scaleMat = DirectX::XMMatrixScaling(scale, scale, scale);
		DirectX::XMMATRIX transMat = DirectX::XMMatrixTranslation(light.position.x, light.position.y, light.position.z);
		DirectX::XMMATRIX worldMat = scaleMat * rotMat * transMat;

		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 worldInvTrans;
		XMStoreFloat4x4(&world, worldMat);
		XMStoreFloat4x4(&worldInvTrans, XMMatrixInverse(0, XMMatrixTranspose(worldMat)));

		// Set up the world matrix for this light
		lightVS->SetMatrix4x4("world", world);
		lightVS->SetMatrix4x4("worldInverseTranspose", worldInvTrans);

		// Set up the pixel shader data
		DirectX::XMFLOAT3 finalColor = light.color;
		finalColor.x *= light.intensity;
		finalColor.y *= light.intensity;
		finalColor.z *= light.intensity;
		lightPS->SetFloat3("Color", finalColor);

		// Copy data
		lightVS->CopyAllBufferData();
		lightPS->CopyAllBufferData();

		// Draw
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, globalAssets.globalMeshes["Sphere"]->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(globalAssets.globalMeshes["Sphere"]->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(
			globalAssets.globalMeshes["Sphere"]->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}

}


void Renderer::RenderShadows(std::shared_ptr<Camera> shadowCam, int depthBufferIndex) {
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDepth;
	shadowDepth = shadowDepthBuffers[depthBufferIndex];

	context->OMSetRenderTargets(0, 0, shadowDepth.Get());

	context->ClearDepthStencilView(shadowDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	context->RSSetState(shadowRasterizer.Get());

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (float)shadowSize;
	vp.Height = (float)shadowSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	context->PSSetShader(0, 0, 0);
	VSShadow->SetShader();

	VSShadow->SetMatrix4x4("view", shadowCam->GetViewMatrix());
	VSShadow->SetMatrix4x4("projection", shadowCam->GetProjectionMatrix());

	std::map<std::string, std::shared_ptr<GameEntity>>::iterator it;

	for (it = globalAssets.globalEntities.begin(); it != globalAssets.globalEntities.end(); it++) {
		VSShadow->SetMatrix4x4("world", it->second->GetTransform()->GetWorldMatrix());

		VSShadow->CopyAllBufferData();

		//globalAssets.globalEntities[i]->Draw(context, cam, nullptr, nullptr);
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, it->second->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(it->second->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(
			it->second->GetMesh()->GetIndexCount(),
			0,     
			0);    
	}

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	vp.Width = windowWidth;
	vp.Height = windowHeight;
	context->RSSetViewports(1, &vp);

	context->RSSetState(0);
}

void Renderer::Draw(std::shared_ptr<Camera> cam) {

	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.4f, 0.6f, 0.75f, 0.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
	for (int i = 0; i < 6; i++) {
		context->ClearRenderTargetView(ssaoRTVs->Get(), color);
	}
	const float depths[4] = { 1,0,0,0 };
	context->ClearRenderTargetView(ssaoRTVs[3].Get(), depths);

	ID3D11RenderTargetView* renderTargets[4] = {};
	for (int i = 0; i < 4; i++) {
		renderTargets[i] = ssaoRTVs[i].Get();
	}
	context->OMSetRenderTargets(4, renderTargets, depthBufferDSV.Get());

	//For now, it's more optimized to set const values such as lights out here.
	std::shared_ptr<SimplePixelShader> pixShader = globalAssets.globalEntities.at("Bronze Cube")->GetMaterial()->GetPixShader();
	unsigned int lightCount = globalAssets.globalLights.size();
	pixShader->SetShader();
	pixShader->SetData("lights", globalAssets.globalLights.data(), sizeof(Light) * 64);
	pixShader->SetData("lightCount", &lightCount, sizeof(lightCount));
	pixShader->SetFloat3("ambientColor", ambientColor);

	// Set buffers in the input assembler
	//  - Do this ONCE PER OBJECT you're drawing, since each object might
	//    have different geometry.
	std::map<std::string, std::shared_ptr<GameEntity>>::iterator it;

	for (it = globalAssets.globalEntities.begin(); it != globalAssets.globalEntities.end(); it++) {
		//Calculate pixel lighting before draw
		pixShader = it->second->GetMaterial()->GetPixShader();
		pixShader->SetShader();
		pixShader->SetFloat("uvMult", it->second->GetMaterial()->GetTiling());
		pixShader->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
		pixShader->SetSamplerState("sampleState", it->second->GetMaterial()->GetSamplerState().Get());
		pixShader->SetSamplerState("clampSampler", it->second->GetMaterial()->GetClampSamplerState().Get());
		pixShader->SetShaderResourceView("textureAlbedo", it->second->GetMaterial()->GetTexture().Get());
		pixShader->SetShaderResourceView("textureRough", it->second->GetMaterial()->GetRoughMap().Get());
		pixShader->SetShaderResourceView("textureMetal", it->second->GetMaterial()->GetMetalMap().Get());
		if (it->second->GetMaterial()->GetNormalMap() != nullptr) {
			pixShader->SetShaderResourceView("textureNormal", it->second->GetMaterial()->GetNormalMap().Get());
		}
		pixShader->SetShaderResourceView("shadowMap", shadowSRV.Get());
		pixShader->SetSamplerState("shadowState", shadowSampler.Get());
		pixShader->SetShaderResourceView("envShadowMap", envShadowSRV.Get());

		pixShader->SetInt("specIBLTotalMipLevels", currentSky->GetIBLMipLevelCount());
		pixShader->SetShaderResourceView("irradianceIBLMap", currentSky->GetIrradianceCubeMap().Get());
		pixShader->SetShaderResourceView("brdfLookUpMap", currentSky->GetBRDFLookupTexture().Get());
		pixShader->SetShaderResourceView("specularIBLMap", currentSky->GetConvolvedSpecularCubeMap().Get());

		pixShader->CopyAllBufferData();

		it->second->Draw(context, cam, flashShadowCamera, mainShadowCamera);
	}

	//Now deal with rendering the terrain, PS data first
	std::shared_ptr<SimplePixelShader> PSTerrain = globalAssets.pixelShaders.at("TerrainPS");
	std::shared_ptr<SimpleVertexShader> VSTerrain = globalAssets.vertexShaders.at("TerrainVS");
	std::shared_ptr<GameEntity> terrainEntity = globalAssets.globalTerrainEntities.at("Main Terrain");
	PSTerrain->SetShader();
	PSTerrain->SetData("lights", globalAssets.globalLights.data(), sizeof(Light) * 64);
	PSTerrain->SetData("lightCount", &lightCount, sizeof(unsigned int));
	PSTerrain->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
	PSTerrain->SetFloat("uvMultNear", 50.0f);
	PSTerrain->SetFloat("uvMultFar", 150.0f);
	PSTerrain->SetShaderResourceView("shadowMap", shadowSRV.Get());
	PSTerrain->SetShaderResourceView("blendMap", globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMap.Get());
	PSTerrain->SetSamplerState("shadowState", shadowSampler.Get());
	PSTerrain->SetSamplerState("clampSampler", globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMaterials[0].GetClampSamplerState().Get());
	PSTerrain->SetShaderResourceView("envShadowMap", envShadowSRV.Get());

	for (int i = 0; i < globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMaterials.size(); i++) {
		std::string a = "texture" + std::to_string(i + 1) + "Albedo";
		std::string n = "texture" + std::to_string(i + 1) + "Normal";
		std::string r = "texture" + std::to_string(i + 1) + "Rough";
		std::string m = "texture" + std::to_string(i + 1) + "Metal";
		PSTerrain->SetShaderResourceView(a, globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMaterials[i].GetTexture().Get());
		PSTerrain->SetShaderResourceView(n, globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMaterials[i].GetNormalMap().Get());
		PSTerrain->SetShaderResourceView(r, globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMaterials[i].GetRoughMap().Get());
		PSTerrain->SetShaderResourceView(m, globalAssets.globalTerrainMaterials.at("Forest TMaterial")->blendMaterials[i].GetMetalMap().Get());

		PSTerrain->SetInt("specIBLTotalMipLevels", currentSky->GetIBLMipLevelCount());
		PSTerrain->SetShaderResourceView("irradianceIBLMap", currentSky->GetIrradianceCubeMap().Get());
		PSTerrain->SetShaderResourceView("brdfLookUpMap", currentSky->GetBRDFLookupTexture().Get());
		PSTerrain->SetShaderResourceView("specularIBLMap", currentSky->GetConvolvedSpecularCubeMap().Get());
	}

	PSTerrain->CopyAllBufferData();
	
	terrainEntity->DrawFromVerts(context, VSTerrain, cam, flashShadowCamera, mainShadowCamera);

    this->currentSky->Draw(context, cam);

	std::shared_ptr<SimpleVertexShader> fullscreenVS = globalAssets.vertexShaders.at("FullscreenVS");
	fullscreenVS->SetShader();

	// SSAO Rendering
	// First step is the initial SSAO render pass
	renderTargets[0] = ssaoRTVs[4].Get();
	renderTargets[1] = 0;
	renderTargets[2] = 0;
	renderTargets[3] = 0;
	context->OMSetRenderTargets(4, renderTargets, 0);

	std::shared_ptr<SimplePixelShader> ssaoPS = globalAssets.pixelShaders.at("SSAOPS");
	ssaoPS->SetShader();

	// Inverse projection matrix
	XMFLOAT4X4 invProj;
	XMFLOAT4X4 view = mainCamera->GetViewMatrix();
	XMFLOAT4X4 proj = mainCamera->GetProjectionMatrix();

	XMStoreFloat4x4(&invProj, XMMatrixInverse(0, XMLoadFloat4x4(&proj)));
	ssaoPS->SetMatrix4x4("invProjection", invProj);
	ssaoPS->SetMatrix4x4("projection", proj);
	ssaoPS->SetMatrix4x4("view", view);
	ssaoPS->SetData("offsets", ssaoOffsets, sizeof(XMFLOAT4) * ARRAYSIZE(ssaoOffsets));
	ssaoPS->SetFloat("ssaoRadius", ssaoRadius);
	ssaoPS->SetInt("ssaoSamples", ssaoSamples);
	ssaoPS->SetFloat2("randomTextureScreenScale", XMFLOAT2(windowWidth / 4.0f, windowHeight / 4.0f));
	ssaoPS->CopyAllBufferData();

	ssaoPS->SetShaderResourceView("normals", ssaoRTVs[2].Get());
	ssaoPS->SetShaderResourceView("depths", ssaoRTVs[3].Get());
	ssaoPS->SetShaderResourceView("random", ssaoRandomSRV.Get());

	context->Draw(3, 0);

	renderTargets[0] = ssaoRTVs[5].Get();
	context->OMSetRenderTargets(1, renderTargets, 0);

	//Do I need to set a sampler state? They all use them but I don't think I'm passing anything rn
	std::shared_ptr<SimplePixelShader> ssaoBlurPS = globalAssets.pixelShaders.at("SSAOBlurPS");
	ssaoBlurPS->SetShader();
	ssaoBlurPS->SetShaderResourceView("sceneColorsNoAmbient", ssaoRTVs[0].Get());
	ssaoBlurPS->SetShaderResourceView("ambient", ssaoRTVs[1].Get());
	ssaoBlurPS->SetShaderResourceView("SSAOBlur", ssaoRTVs[5].Get());
	ssaoBlurPS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ssaoBlurPS->CopyAllBufferData();
	context->Draw(3, 0);

	// Combine all results
	renderTargets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, renderTargets, 0);

	std::shared_ptr<SimplePixelShader> ssaoCombinePS = globalAssets.pixelShaders.at("SSAOCombinePS");
	ssaoCombinePS->SetShader();
	ssaoCombinePS->SetShaderResourceView("sceneColorsNoAmbient", ssaoRTVs[0].Get());
	ssaoCombinePS->SetShaderResourceView("ambient", ssaoRTVs[1].Get());
	ssaoCombinePS->SetShaderResourceView("SSAOBlur", ssaoRTVs[5].Get());
	ssaoCombinePS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ssaoCombinePS->CopyAllBufferData();
	context->Draw(3, 0);

	// Draw the point light
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	DrawPointLights();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	// Unbind all in-use shader resources
	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);
}

void Renderer::SetActiveSky(std::shared_ptr<Sky> sky) {
    this->currentSky = sky;
}