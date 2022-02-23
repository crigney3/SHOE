#include "../Headers/Renderer.h"

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
	this->mainCamera = globalAssets.GetCameraByName("mainCamera");
	this->mainShadowCamera = globalAssets.GetCameraByName("mainShadowCamera");
	this->flashShadowCamera = globalAssets.GetCameraByName("flashShadowCamera");

	PostResize(windowHeight, windowWidth, backBufferRTV, depthBufferDSV);
}

Renderer::~Renderer(){

}

void Renderer::InitRenderTargetViews() {
	for (int i = 0; i < 3; i++) {
		ssaoTexture2D[i].Reset();
		renderTargetRTVs[i].Reset();
		renderTargetSRVs[i].Reset();

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
		device->CreateRenderTargetView(ssaoTexture2D[i].Get(), &rtvDesc, renderTargetRTVs[i].GetAddressOf());

		device->CreateShaderResourceView(ssaoTexture2D[i].Get(), 0, renderTargetSRVs[i].GetAddressOf());
	}

	// Initialize Depths as just a float texture
	for (int i = 3; i < 6; i++) {
		ssaoTexture2D[i].Reset();
		renderTargetRTVs[i].Reset();
		renderTargetSRVs[i].Reset();

		D3D11_TEXTURE2D_DESC basicTexDesc = {};
		basicTexDesc.Width = windowWidth;
		basicTexDesc.Height = windowHeight;
		basicTexDesc.ArraySize = 1;
		basicTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		basicTexDesc.Format = DXGI_FORMAT_R32_FLOAT;
		basicTexDesc.MipLevels = 1;
		basicTexDesc.MiscFlags = 0;
		basicTexDesc.SampleDesc.Count = 1;
		device->CreateTexture2D(&basicTexDesc, 0, &ssaoTexture2D[i]);

		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Format = basicTexDesc.Format;
		device->CreateRenderTargetView(ssaoTexture2D[i].Get(), &rtvDesc, renderTargetRTVs[i].GetAddressOf());

		device->CreateShaderResourceView(ssaoTexture2D[i].Get(), 0, renderTargetSRVs[i].GetAddressOf());
	}

	compositeTexture.Reset();
	renderTargetRTVs[RTVTypes::COMPOSITE].Reset();
	renderTargetSRVs[RTVTypes::COMPOSITE].Reset();
	D3D11_TEXTURE2D_DESC compositeTexDesc = {};
	compositeTexDesc.Width = windowWidth;
	compositeTexDesc.Height = windowHeight;
	compositeTexDesc.ArraySize = 1;
	compositeTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	compositeTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	compositeTexDesc.MipLevels = 1;
	compositeTexDesc.MiscFlags = 0;
	compositeTexDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&compositeTexDesc, 0, compositeTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC compositeRTVDesc = {};
	compositeRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	compositeRTVDesc.Texture2D.MipSlice = 0;
	compositeRTVDesc.Format = compositeTexDesc.Format;
	device->CreateRenderTargetView(compositeTexture.Get(), &compositeRTVDesc, renderTargetRTVs[RTVTypes::COMPOSITE].GetAddressOf());

	device->CreateShaderResourceView(compositeTexture.Get(), 0, renderTargetSRVs[RTVTypes::COMPOSITE].GetAddressOf());

	silhouetteTexture.Reset();
	renderTargetRTVs[RTVTypes::REFRACTION_SILHOUETTE].Reset();
	renderTargetSRVs[RTVTypes::REFRACTION_SILHOUETTE].Reset();
	D3D11_TEXTURE2D_DESC silhouetteTexDesc = {};
	silhouetteTexDesc.Width = windowWidth;
	silhouetteTexDesc.Height = windowHeight;
	silhouetteTexDesc.ArraySize = 1;
	silhouetteTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	silhouetteTexDesc.Format = DXGI_FORMAT_R8_UNORM;
	silhouetteTexDesc.MipLevels = 1;
	silhouetteTexDesc.MiscFlags = 0;
	silhouetteTexDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&silhouetteTexDesc, 0, silhouetteTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC silhouetteRTVDesc = {};
	silhouetteRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	silhouetteRTVDesc.Texture2D.MipSlice = 0;
	silhouetteRTVDesc.Format = silhouetteTexDesc.Format;
	device->CreateRenderTargetView(silhouetteTexture.Get(), &silhouetteRTVDesc, renderTargetRTVs[RTVTypes::REFRACTION_SILHOUETTE].GetAddressOf());

	device->CreateShaderResourceView(silhouetteTexture.Get(), 0, renderTargetSRVs[RTVTypes::REFRACTION_SILHOUETTE].GetAddressOf());


	for (int i = 0; i < MISC_EFFECT_SRV_COUNT; i++) {
		miscEffectDepthBuffers[i].Reset();
		miscEffectSRVs[i].Reset();
	}

	transparentEntities = std::vector<std::shared_ptr<GameEntity>>();
	
	// SSAO needs a random 4x4 texture to work
	const int textureSize = 4;
	const int totalPixels = textureSize * textureSize;
	XMFLOAT4 randomPixels[totalPixels] = {};
	for (int i = 0; i < totalPixels; i++) {
		XMVECTOR randomVec = XMVectorSet(RandomRange(-1, 1), RandomRange(-1, 1), 0, 0);
		XMStoreFloat4(&randomPixels[i], XMVector3Normalize(randomVec));
	}

	// Need to pass this texture to GPU
	ssaoRandomTex.Reset();
	ssaoRandomSRV.Reset();

	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = 4;
	texDesc.Height = 4;
	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.MipLevels = 1;
	texDesc.MiscFlags = 0;
	texDesc.SampleDesc.Count = 1;

	D3D11_SUBRESOURCE_DATA data = {};
	data.pSysMem = randomPixels;
	data.SysMemPitch = sizeof(XMFLOAT4) * 4;

	device->CreateTexture2D(&texDesc, &data, ssaoRandomTex.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = texDesc.Format;
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

	// Set up particle depth and blending
	particleDepthState.Reset();
	particleBlendAdditive.Reset();

	D3D11_DEPTH_STENCIL_DESC particleDepthDesc = {};
	particleDepthDesc.DepthEnable = true; 
	particleDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; 
	particleDepthDesc.DepthFunc = D3D11_COMPARISON_LESS;
	device->CreateDepthStencilState(&particleDepthDesc, particleDepthState.GetAddressOf());

	D3D11_BLEND_DESC additiveBlendDesc = {};
	additiveBlendDesc.RenderTarget[0].BlendEnable = true;
	additiveBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD; // Add both colors
	additiveBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD; // Add both alpha values
	additiveBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;   // 100% of source color
	additiveBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;  // 100% of destination color
	additiveBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;   // 100% of source alpha
	additiveBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;  // 100% of destination alpha
	additiveBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	device->CreateBlendState(&additiveBlendDesc, particleBlendAdditive.GetAddressOf());

	// Set up refraction silhouette depth buffer
	refractionSilhouetteDepthState.Reset();
	prePassDepthState.Reset();

	D3D11_DEPTH_STENCIL_DESC rsDepthDesc = {};
	rsDepthDesc.DepthEnable = true;
	rsDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No depth writing
	rsDepthDesc.DepthFunc = D3D11_COMPARISON_LESS; // Get only the closest pixels
	device->CreateDepthStencilState(&rsDepthDesc, refractionSilhouetteDepthState.GetAddressOf());

	// Set up prepass depth buffer
	D3D11_DEPTH_STENCIL_DESC prePassDepthDesc = {};
	prePassDepthDesc.DepthEnable = true;
	prePassDepthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // No depth writing - unsure whether to keep this here
	prePassDepthDesc.DepthFunc = D3D11_COMPARISON_LESS; // Get only the closest pixels
	device->CreateDepthStencilState(&prePassDepthDesc, prePassDepthState.GetAddressOf());

	InitShadows();
}

void Renderer::InitShadows() {
    //Set up buffers and data for shadows
	shadowSize = 2048;

	shadowRasterizer.Reset();
	shadowSampler.Reset();
	this->VSShadow.reset();

	this->VSShadow = globalAssets.GetVertexShaderByName("ShadowVS");

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

	D3D11_TEXTURE2D_DESC miscDepthDesc = {};
	miscDepthDesc.Width = windowWidth;
	miscDepthDesc.Height = windowHeight;
	miscDepthDesc.ArraySize = 1;
	miscDepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	miscDepthDesc.CPUAccessFlags = 0;
	miscDepthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	miscDepthDesc.MipLevels = 1;
	miscDepthDesc.MiscFlags = 0;
	miscDepthDesc.SampleDesc.Count = 1;
	miscDepthDesc.SampleDesc.Quality = 0;
	miscDepthDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Texture2D* shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, &shadowTexture);

	ID3D11Texture2D* envShadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, &envShadowTexture);

	ID3D11Texture2D* silhouetteTexture;
	device->CreateTexture2D(&miscDepthDesc, 0, &silhouetteTexture);

	ID3D11Texture2D* transparentPrePassTexture;
	device->CreateTexture2D(&miscDepthDesc, 0, &transparentPrePassTexture);

	ID3D11Texture2D* prePassTexture;
	device->CreateTexture2D(&miscDepthDesc, 0, &prePassTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::FLASHLIGHT_SHADOW].GetAddressOf());

	device->CreateDepthStencilView(envShadowTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::ENV_SHADOW].GetAddressOf());

	device->CreateDepthStencilView(silhouetteTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS].GetAddressOf());

	device->CreateDepthStencilView(transparentPrePassTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS].GetAddressOf());

	device->CreateDepthStencilView(prePassTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS].GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::FLASHLIGHT_SHADOW].GetAddressOf());

	device->CreateShaderResourceView(envShadowTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::ENV_SHADOW].GetAddressOf());

	device->CreateShaderResourceView(silhouetteTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS].GetAddressOf());

	device->CreateShaderResourceView(transparentPrePassTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS].GetAddressOf());

	device->CreateShaderResourceView(prePassTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS].GetAddressOf());

	shadowTexture->Release();
	envShadowTexture->Release();
	silhouetteTexture->Release();
	transparentPrePassTexture->Release();
	prePassTexture->Release();

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
}

void Renderer::PreResize() {
	this->backBufferRTV.Reset();
	this->depthBufferDSV.Reset();
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

	InitRenderTargetViews();
}

// ------------------------------------------------------------------------
// Draws the point lights as solid color spheres - credit to Chris Cascioli
// ------------------------------------------------------------------------
void Renderer::DrawPointLights()
{
	std::shared_ptr<SimpleVertexShader> lightVS = globalAssets.GetVertexShaderByName("BasicVS");
	std::shared_ptr<SimplePixelShader> lightPS = globalAssets.GetPixelShaderByName("SolidColorPS");

	// Turn on these shaders
	lightVS->SetShader();
	lightPS->SetShader();

	// Set up vertex shader
	lightVS->SetMatrix4x4("view", globalAssets.GetCameraByName("mainCamera")->GetViewMatrix());
	lightVS->SetMatrix4x4("projection", globalAssets.GetCameraByName("mainCamera")->GetProjectionMatrix());

	for (int i = 0; i < globalAssets.GetLightArraySize(); i++)
	{
		Light light = globalAssets.GetLightArray()[i];

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
		context->IASetVertexBuffers(0, 1, globalAssets.GetMeshByName("Sphere")->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(globalAssets.GetMeshByName("Sphere")->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(
			globalAssets.GetMeshByName("Sphere")->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}

}

void Renderer::RenderDepths(std::shared_ptr<Camera> sourceCam, MiscEffectSRVTypes type) {
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> miscEffectDepth;
	miscEffectDepth = miscEffectDepthBuffers[type];

	context->RSSetState(0); // Unclear if this should be a custom rasterizer state

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (float)windowWidth;
	vp.Height = (float)windowHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	VSShadow->SetShader();

	VSShadow->SetMatrix4x4("view", sourceCam->GetViewMatrix());
	VSShadow->SetMatrix4x4("projection", sourceCam->GetProjectionMatrix());

	if (type == MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS) {
		std::shared_ptr<SimplePixelShader> solidColorPS = globalAssets.GetPixelShaderByName("SolidColorPS");

		context->OMSetRenderTargets(1, renderTargetRTVs[RTVTypes::REFRACTION_SILHOUETTE].GetAddressOf(), depthBufferDSV.Get());

		//context->ClearDepthStencilView(miscEffectDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		context->OMSetDepthStencilState(refractionSilhouetteDepthState.Get(), 0);

		std::vector<std::shared_ptr<GameEntity>>::iterator it;

		for (it = transparentEntities.begin(); it != transparentEntities.end(); it++) {
			if (!it->get()->GetEnableDisable()) continue;

			// Store the old material's pixel shader
			/*std::shared_ptr<Material> mat = it->get()->GetMaterial();
			std::shared_ptr<SimplePixelShader> prevPS = mat->GetPixShader();
			mat->SetPixelShader(solidColorPS);*/

			// Standard depth pre-pass
			VSShadow->SetMatrix4x4("world", it->get()->GetTransform()->GetWorldMatrix());

			VSShadow->CopyAllBufferData();

			solidColorPS->SetShader();
			solidColorPS->SetFloat3("Color", DirectX::XMFLOAT3(1, 1, 1));
			solidColorPS->CopyAllBufferData();

			//globalAssets.globalEntities[i]->Draw(context, cam, nullptr, nullptr);
			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, it->get()->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(it->get()->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(
				it->get()->GetMesh()->GetIndexCount(),
				0,
				0);

			//mat->SetPixelShader(prevPS);
		}
	}
	else {
		context->OMSetRenderTargets(0, 0, miscEffectDepth.Get());

		context->ClearDepthStencilView(miscEffectDepth.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

		context->PSSetShader(0, 0, 0);

		std::vector<std::shared_ptr<GameEntity>>::iterator it;

		for (it = globalAssets.GetActiveGameEntities()->begin(); it != globalAssets.GetActiveGameEntities()->end(); it++) {
			if (!it->get()->GetEnableDisable()) continue;

			// Standard depth pre-pass
			VSShadow->SetMatrix4x4("world", it->get()->GetTransform()->GetWorldMatrix());

			VSShadow->CopyAllBufferData();

			//globalAssets.globalEntities[i]->Draw(context, cam, nullptr, nullptr);
			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, it->get()->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(it->get()->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(
				it->get()->GetMesh()->GetIndexCount(),
				0,
				0);
		}
	}

	context->OMSetDepthStencilState(0, 0);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	vp.Width = (float)windowWidth;
	vp.Height = (float)windowHeight;
	context->RSSetViewports(1, &vp);

	//context->RSSetState(0);
}

void Renderer::RenderShadows(std::shared_ptr<Camera> shadowCam, MiscEffectSRVTypes type) {
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDepth;
	shadowDepth = miscEffectDepthBuffers[type];

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

	std::vector<std::shared_ptr<GameEntity>>::iterator it;

	for (it = globalAssets.GetActiveGameEntities()->begin(); it != globalAssets.GetActiveGameEntities()->end(); it++) {
		if (!it->get()->GetEnableDisable() | it->get()->GetMaterial()->GetTransparent()) continue;

		// This is similar to what I'd need for any depth pre-pass
		VSShadow->SetMatrix4x4("world", it->get()->GetTransform()->GetWorldMatrix());

		VSShadow->CopyAllBufferData();

		//globalAssets.globalEntities[i]->Draw(context, cam, nullptr, nullptr);
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, it->get()->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(it->get()->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(
			it->get()->GetMesh()->GetIndexCount(),
			0,     
			0);    
	}

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	vp.Width = (float)windowWidth;
	vp.Height = (float)windowHeight;
	context->RSSetViewports(1, &vp);

	context->RSSetState(0);
}

void Renderer::Draw(std::shared_ptr<Camera> cam, float totalTime) {

	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthBufferDSV.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);
	for (int i = 0; i < RTVTypes::RTV_TYPE_COUNT; i++) {
		context->ClearRenderTargetView(renderTargetRTVs[i].Get(), color);
	}
	const float depths[4] = { 1,0,0,0 };
	context->ClearRenderTargetView(renderTargetRTVs[RTVTypes::DEPTHS].Get(), depths);

	// This fills in all renderTargets used before post-processing
	// Includes Colors, Ambient, Normals, and Depths
	const int numTargets = 4;
	static ID3D11RenderTargetView* renderTargets[numTargets] = {};
	for (int i = 0; i < numTargets; i++) {
		renderTargets[i] = renderTargetRTVs[i].Get();
	}
	context->OMSetRenderTargets(4, renderTargets, depthBufferDSV.Get());

	// Per Frame data can be set out here for optimization
	// This section could be improved, see Chris's Demos and
	// Structs in header. Currently only supports the single default 
	// PBR+IBL shader
	unsigned int lightCount = globalAssets.GetLightArraySize();

	std::shared_ptr<SimpleVertexShader> perFrameVS = globalAssets.GetVertexShaderByName("NormalsVS");

	perFrameVS->SetMatrix4x4("view", cam->GetViewMatrix());
	perFrameVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	perFrameVS->SetMatrix4x4("lightView", flashShadowCamera->GetViewMatrix());
	perFrameVS->SetMatrix4x4("lightProjection", flashShadowCamera->GetProjectionMatrix());

	perFrameVS->SetMatrix4x4("envLightView", mainShadowCamera->GetViewMatrix());
	perFrameVS->SetMatrix4x4("envLightProjection", mainShadowCamera->GetProjectionMatrix());

	std::shared_ptr<SimplePixelShader> perFramePS = globalAssets.GetPixelShaderByName("NormalsPS");

	perFramePS->SetData("lights", globalAssets.GetLightArray(), sizeof(Light) * 64);
	perFramePS->SetData("lightCount", &lightCount, sizeof(lightCount));
	perFramePS->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
	if (this->currentSky->GetEnableDisable()) {
		perFramePS->SetInt("specIBLTotalMipLevels", currentSky->GetIBLMipLevelCount());
	}

	// Set buffers in the input assembler
	//  - Do this ONCE PER OBJECT you're drawing, since each object might
	//    have different geometry.
	std::vector<std::shared_ptr<GameEntity>>::iterator it;

	// Track these to skip and render last
	// Includes refractive entities, since those will
	// always be transparent
	transparentEntities.clear();

	// Material-Sort Rendering:
	// We can assume the list is sorted by this step.
	// To potentially save operations, track a current Material and Mesh
	// A note that this list is NOT sorted by mesh - current Mesh
	// is tracked on the chance of saving a cycle if two+ concurrent
	// Meshes are the same.
	// VS and PS are tracked for materials that differ, but share
	// shaders with other objects.

	SimpleVertexShader* currentVS = 0;
	SimplePixelShader* currentPS = 0;
	Material* currentMaterial = 0;
	Mesh* currentMesh = 0;

	// TODO : Change to auto instead of iterator
	for (it = globalAssets.GetActiveGameEntities()->begin(); it != globalAssets.GetActiveGameEntities()->end(); it++) {
		if (!it->get()->GetEnableDisable()) continue;

		if (it->get()->GetMaterial()->GetTransparent()) {
			// Iterator is being difficult about returning a smart pointer
			// Using distance to get around this
			// It's cursed, I know
			transparentEntities.push_back(globalAssets.GetGameEntityByID(std::distance(globalAssets.GetActiveGameEntities()->begin(), it)));
			continue;
		}

		std::shared_ptr<Material> itMatPtr = it->get()->GetMaterial();
		std::shared_ptr<Mesh> itMeshPtr = it->get()->GetMesh();
		Transform* itTransPtr = it->get()->GetTransform();

		if (currentMaterial != itMatPtr.get())
		{
			// Eventual improvement:
			// Move all VS and PS "Set" calls into Material
			// This would require storing a lot more data in material
			// With shadows, it would also require passing in a lot of data
			// And handling edge cases like main camera swaps

			currentMaterial = itMatPtr.get();

			if (currentVS != currentMaterial->GetVertShader().get()) {
				// Set new Shader and copy per-frame data
				currentVS = currentMaterial->GetVertShader().get();
				currentVS->SetShader();

				perFrameVS->CopyBufferData("PerFrame");
			}

			if (currentPS != currentMaterial->GetPixShader().get()) {
				// Set new Shader and copy per-frame data
				currentPS = currentMaterial->GetPixShader().get();
				currentPS->SetShader();

				perFramePS->CopyBufferData("PerFrame");
			}

			// Per-Material VS Data
			currentVS->SetFloat4("colorTint", currentMaterial->GetTint());

			currentVS->CopyBufferData("PerMaterial");

			// Per-Material PS Data
			currentPS->SetFloat("uvMult", itMatPtr->GetTiling());

			currentPS->CopyBufferData("PerMaterial");

			// Set textures and samplers
			currentPS->SetSamplerState("sampleState", itMatPtr->GetSamplerState().Get());
			currentPS->SetSamplerState("clampSampler", itMatPtr->GetClampSamplerState().Get());
			currentPS->SetShaderResourceView("textureAlbedo", itMatPtr->GetTexture().Get());
			currentPS->SetShaderResourceView("textureRough", itMatPtr->GetRoughMap().Get());
			currentPS->SetShaderResourceView("textureMetal", itMatPtr->GetMetalMap().Get());
			if (itMatPtr->GetNormalMap() != nullptr) {
				currentPS->SetShaderResourceView("textureNormal", itMatPtr->GetNormalMap().Get());
			}
			currentPS->SetShaderResourceView("shadowMap", miscEffectSRVs[MiscEffectSRVTypes::FLASHLIGHT_SHADOW].Get());
			currentPS->SetSamplerState("shadowState", shadowSampler.Get());
			currentPS->SetShaderResourceView("envShadowMap", miscEffectSRVs[MiscEffectSRVTypes::ENV_SHADOW].Get());

			if (this->currentSky->GetEnableDisable()) {
				currentPS->SetShaderResourceView("irradianceIBLMap", currentSky->GetIrradianceCubeMap().Get());
				currentPS->SetShaderResourceView("brdfLookUpMap", currentSky->GetBRDFLookupTexture().Get());
				currentPS->SetShaderResourceView("specularIBLMap", currentSky->GetConvolvedSpecularCubeMap().Get());
			}
		}

		if (currentMesh != itMeshPtr.get()) {
			currentMesh = itMeshPtr.get();

			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, itMeshPtr->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(itMeshPtr->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		}

		if (currentVS != 0) {
			// Per-Object data
			currentVS->SetMatrix4x4("world", itTransPtr->GetWorldMatrix());

			currentVS->CopyBufferData("PerObject");
		}

		if (currentMesh != 0) {
			context->DrawIndexed(itMeshPtr->GetIndexCount(), 0, 0);
		}
	}

	//Now deal with rendering the terrain, PS data first
	std::shared_ptr<GameEntity> terrainEntity = globalAssets.GetTerrainByName("Main Terrain");
	if (terrainEntity->GetEnableDisable()) {
		std::shared_ptr<SimplePixelShader> PSTerrain = globalAssets.GetPixelShaderByName("TerrainPS");
		std::shared_ptr<SimpleVertexShader> VSTerrain = globalAssets.GetVertexShaderByName("TerrainVS");
		PSTerrain->SetShader();
		PSTerrain->SetData("lights", globalAssets.GetLightArray(), sizeof(Light) * 64);
		PSTerrain->SetData("lightCount", &lightCount, sizeof(unsigned int));
		PSTerrain->SetFloat3("cameraPos", cam->GetTransform()->GetPosition());
		PSTerrain->SetFloat("uvMultNear", 50.0f);
		PSTerrain->SetFloat("uvMultFar", 150.0f);
		PSTerrain->SetShaderResourceView("shadowMap", miscEffectSRVs[MiscEffectSRVTypes::FLASHLIGHT_SHADOW].Get());
		PSTerrain->SetShaderResourceView("blendMap", globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMap.Get());
		PSTerrain->SetSamplerState("shadowState", shadowSampler.Get());
		PSTerrain->SetSamplerState("clampSampler", globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMaterials[0].GetClampSamplerState().Get());
		PSTerrain->SetShaderResourceView("envShadowMap", miscEffectSRVs[MiscEffectSRVTypes::ENV_SHADOW].Get());

		for (int i = 0; i < globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMaterials.size(); i++) {
			std::string a = "texture" + std::to_string(i + 1) + "Albedo";
			std::string n = "texture" + std::to_string(i + 1) + "Normal";
			std::string r = "texture" + std::to_string(i + 1) + "Rough";
			std::string m = "texture" + std::to_string(i + 1) + "Metal";
			PSTerrain->SetShaderResourceView(a, globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMaterials[i].GetTexture().Get());
			PSTerrain->SetShaderResourceView(n, globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMaterials[i].GetNormalMap().Get());
			PSTerrain->SetShaderResourceView(r, globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMaterials[i].GetRoughMap().Get());
			PSTerrain->SetShaderResourceView(m, globalAssets.GetTerrainMaterialByName("Forest TMaterial")->blendMaterials[i].GetMetalMap().Get());
		}

		if (this->currentSky->GetEnableDisable()) {
			PSTerrain->SetInt("specIBLTotalMipLevels", currentSky->GetIBLMipLevelCount());
			PSTerrain->SetShaderResourceView("irradianceIBLMap", currentSky->GetIrradianceCubeMap().Get());
			PSTerrain->SetShaderResourceView("brdfLookUpMap", currentSky->GetBRDFLookupTexture().Get());
			PSTerrain->SetShaderResourceView("specularIBLMap", currentSky->GetConvolvedSpecularCubeMap().Get());
		}

		PSTerrain->CopyAllBufferData();

		terrainEntity->DrawFromVerts(context, VSTerrain, cam, flashShadowCamera, mainShadowCamera);
	}

	if (this->currentSky->GetEnableDisable()) {
		this->currentSky->Draw(context, cam);
	}
    
	std::shared_ptr<SimpleVertexShader> fullscreenVS = globalAssets.GetVertexShaderByName("FullscreenVS");
	fullscreenVS->SetShader();

	// SSAO Rendering
	// First step is the initial SSAO render pass
	renderTargets[0] = renderTargetRTVs[RTVTypes::SSAO_RAW].Get();
	renderTargets[1] = 0;
	renderTargets[2] = 0;
	renderTargets[3] = 0;
	context->OMSetRenderTargets(4, renderTargets, 0);

	std::shared_ptr<SimplePixelShader> ssaoPS = globalAssets.GetPixelShaderByName("SSAOPS");
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

	ssaoPS->SetShaderResourceView("normals", renderTargetSRVs[RTVTypes::NORMALS].Get());
	ssaoPS->SetShaderResourceView("depths", renderTargetSRVs[RTVTypes::DEPTHS].Get());
	ssaoPS->SetShaderResourceView("random", ssaoRandomSRV.Get());

	context->Draw(3, 0);

	renderTargets[0] = renderTargetRTVs[RTVTypes::SSAO_BLUR].Get();
	context->OMSetRenderTargets(1, renderTargets, 0);

	std::shared_ptr<SimplePixelShader> ssaoBlurPS = globalAssets.GetPixelShaderByName("SSAOBlurPS");
	ssaoBlurPS->SetShader();
	ssaoBlurPS->SetShaderResourceView("SSAO", renderTargetSRVs[RTVTypes::SSAO_RAW].Get());
	ssaoBlurPS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ssaoBlurPS->CopyAllBufferData();
	context->Draw(3, 0);

	// Refractive and Transparent objects are drawn here
	// This uses refraction silhouette techniques, as well as
	// the depth pre-pass from earlier in Draw

	if (!transparentEntities.empty())
	{
		// Combine all results into the Composite buffer
		renderTargets[0] = renderTargetRTVs[RTVTypes::COMPOSITE].Get();
		context->OMSetRenderTargets(1, renderTargets, 0);

		std::shared_ptr<SimplePixelShader> ssaoCombinePS = globalAssets.GetPixelShaderByName("SSAOCombinePS");
		ssaoCombinePS->SetShader();
		ssaoCombinePS->SetShaderResourceView("sceneColorsNoAmbient", renderTargetSRVs[RTVTypes::COLORS_NO_AMBIENT].Get());
		ssaoCombinePS->SetShaderResourceView("ambient", renderTargetSRVs[RTVTypes::COLORS_AMBIENT].Get());
		ssaoCombinePS->SetShaderResourceView("SSAOBlur", renderTargetSRVs[RTVTypes::SSAO_BLUR].Get());
		//ssaoCombinePS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
		ssaoCombinePS->CopyAllBufferData();
		context->Draw(3, 0);

		// For the refraction merge, we need to store the composite
		// to a buffer that can be read by the GPU
		renderTargets[0] = backBufferRTV.Get();
		context->OMSetRenderTargets(1, renderTargets, 0);
		std::shared_ptr<SimplePixelShader> ps = globalAssets.GetPixelShaderByName("TextureSamplePS");
		ps->SetShader();
		ps->SetShaderResourceView("Pixels", renderTargetSRVs[RTVTypes::COMPOSITE].Get());
		ps->SetSamplerState("BasicSampler", transparentEntities[0]->GetMaterial()->GetSamplerState());
		context->Draw(3, 0);

		// First, create the refraction silhouette
		RenderDepths(mainCamera, MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS);

		// Then loop through and draw refractive objects
		renderTargets[0] = backBufferRTV.Get();
		context->OMSetRenderTargets(1, renderTargets, depthBufferDSV.Get());

		// Currently, all refractive shaders are the same, so this is fine
		std::shared_ptr<SimplePixelShader> refractivePS = transparentEntities[0]->GetMaterial()->GetRefractivePixelShader();
		refractivePS->SetShader();

		refractivePS->SetFloat2("screenSize", XMFLOAT2((float)windowWidth, (float)windowHeight));
		refractivePS->SetMatrix4x4("viewMatrix", mainCamera->GetViewMatrix());
		refractivePS->SetMatrix4x4("projMatrix", mainCamera->GetProjectionMatrix());
		refractivePS->SetFloat3("cameraPos", mainCamera->GetTransform()->GetPosition());

		refractivePS->CopyBufferData("PerFrame");

		std::shared_ptr<SimpleVertexShader> refractiveVS = transparentEntities[0]->GetMaterial()->GetVertShader();

		refractiveVS->SetShader();

		refractiveVS->SetMatrix4x4("view", cam->GetViewMatrix());
		refractiveVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());
		refractiveVS->SetMatrix4x4("lightView", flashShadowCamera->GetViewMatrix());
		refractiveVS->SetMatrix4x4("lightProjection", flashShadowCamera->GetProjectionMatrix());

		refractiveVS->SetMatrix4x4("envLightView", mainShadowCamera->GetViewMatrix());
		refractiveVS->SetMatrix4x4("envLightProjection", mainShadowCamera->GetProjectionMatrix());

		refractiveVS->CopyBufferData("PerFrame");

		for (auto ge : transparentEntities) {
			refractiveVS->SetFloat4("colorTint", ge->GetMaterial()->GetTint());

			refractiveVS->CopyBufferData("PerMaterial");

			refractiveVS->SetMatrix4x4("world", ge->GetTransform()->GetWorldMatrix());

			refractiveVS->CopyBufferData("PerObject");

			refractivePS->SetFloat("uvMult", ge->GetMaterial()->GetTiling());
			refractivePS->SetFloat("indexOfRefraction", ge->GetMaterial()->GetIndexOfRefraction());
			refractivePS->SetFloat("refractionScale", ge->GetMaterial()->GetRefractionScale());

			refractivePS->CopyBufferData("PerMaterial");

			refractivePS->SetShaderResourceView("screenPixels", renderTargetSRVs[RTVTypes::COMPOSITE].Get());
			refractivePS->SetShaderResourceView("refractionSilhouette", renderTargetSRVs[RTVTypes::REFRACTION_SILHOUETTE].Get());
			refractivePS->SetShaderResourceView("textureNormal", ge->GetMaterial()->GetNormalMap());

			UINT stride = sizeof(Vertex);
			UINT offset = 0;
			context->IASetVertexBuffers(0, 1, ge->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(ge->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(ge->GetMesh()->GetIndexCount(), 0, 0);
		}
	}
	else {
		// Combine all results, and since there's nothing transparent,
		// move onto presenting
		renderTargets[0] = backBufferRTV.Get();
		context->OMSetRenderTargets(1, renderTargets, 0);

		std::shared_ptr<SimplePixelShader> ssaoCombinePS = globalAssets.GetPixelShaderByName("SSAOCombinePS");
		ssaoCombinePS->SetShader();
		ssaoCombinePS->SetShaderResourceView("sceneColorsNoAmbient", renderTargetSRVs[RTVTypes::COLORS_NO_AMBIENT].Get());
		ssaoCombinePS->SetShaderResourceView("ambient", renderTargetSRVs[RTVTypes::COLORS_AMBIENT].Get());
		ssaoCombinePS->SetShaderResourceView("SSAOBlur", renderTargetSRVs[RTVTypes::SSAO_BLUR].Get());
		//ssaoCombinePS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
		ssaoCombinePS->CopyAllBufferData();
		context->Draw(3, 0);
	}

	// Draw the point light
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	DrawPointLights();

	renderTargets[0] = backBufferRTV.Get();
	context->OMSetRenderTargets(1, renderTargets, depthBufferDSV.Get());
	
	context->OMSetDepthStencilState(particleDepthState.Get(), 0);

	for (int i = 0; i < globalAssets.GetEmitterArraySize(); i++) {
		if (!globalAssets.GetEmitterAtID(i)->GetEnableDisable()) continue;
		globalAssets.GetEmitterAtID(i)->Draw(mainCamera, totalTime, particleBlendAdditive);
	}

	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->OMSetDepthStencilState(0, 0);

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
	ID3D11ShaderResourceView* nullSRVs[32] = {};
	context->PSSetShaderResources(0, 32, nullSRVs);
}

void Renderer::SetActiveSky(std::shared_ptr<Sky> sky) {
    this->currentSky = sky;
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetRenderTargetSRV(RTVTypes type) {
	return renderTargetSRVs[type];
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetMiscEffectSRV(MiscEffectSRVTypes type) {
	return miscEffectSRVs[type];
}