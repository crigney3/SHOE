#include "../Headers/Renderer.h"
#include "../Headers/MeshRenderer.h"
#include "../Headers/ParticleSystem.h"
#include "..\Headers\ShadowProjector.h"

using namespace DirectX;

// forward declaration for static members
bool Renderer::drawColliders;

Renderer::Renderer(
	unsigned int windowHeight,
	unsigned int windowWidth,
	Microsoft::WRL::ComPtr<ID3D11Device> device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context,
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain,
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferRTV,
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthBufferDSV)
{
	this->windowHeight = windowHeight;
	this->windowWidth = windowWidth;
	this->device = device;
	this->context = context;
	this->swapChain = swapChain;
	this->backBufferRTV = backBufferRTV;
	this->depthBufferDSV = depthBufferDSV;
	this->ambientColor = DirectX::XMFLOAT3(0.05f, 0.05f, 0.1f);

	this->sphereMesh = globalAssets.GetMeshByName("Sphere");
	this->cubeMesh = globalAssets.GetMeshByName("Cube");

	this->basicVS = globalAssets.GetVertexShaderByName("BasicVS");
	this->perFrameVS = globalAssets.GetVertexShaderByName("NormalsVS");
	this->fullscreenVS = globalAssets.GetVertexShaderByName("FullscreenVS");
	this->solidColorPS = globalAssets.GetPixelShaderByName("SolidColorPS");
	this->perFramePS = globalAssets.GetPixelShaderByName("NormalsPS");
	this->textureSamplePS = globalAssets.GetPixelShaderByName("TextureSamplePS");
	this->outlinePS = globalAssets.GetPixelShaderByName("OutlinePS");

	this->ssaoPS = globalAssets.GetPixelShaderByName("SSAOPS");
	this->ssaoBlurPS = globalAssets.GetPixelShaderByName("SSAOBlurPS");
	this->ssaoCombinePS = globalAssets.GetPixelShaderByName("SSAOCombinePS");

	this->drawColliders = true;

	this->selectedEntity = -1;

	//create and store the RS State for drawing colliders
	D3D11_RASTERIZER_DESC colliderRSdesc = {};
	colliderRSdesc.FillMode = D3D11_FILL_WIREFRAME;
	colliderRSdesc.CullMode = D3D11_CULL_NONE;
	colliderRSdesc.DepthClipEnable = true;
	colliderRSdesc.DepthBias = 100;
	colliderRSdesc.DepthBiasClamp = 0.0f;
	colliderRSdesc.SlopeScaledDepthBias = 10.0f;
	device->CreateRasterizerState(&colliderRSdesc, &wireframeRasterizer);

	//Init and store sky data
	D3D11_RASTERIZER_DESC rDescription = {};
	rDescription.FillMode = D3D11_FILL_SOLID;
	rDescription.CullMode = D3D11_CULL_FRONT;

	device->CreateRasterizerState(&rDescription, &skyRasterizer);

	D3D11_DEPTH_STENCIL_DESC depthDescription = {};
	depthDescription.DepthEnable = true;
	depthDescription.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	device->CreateDepthStencilState(&depthDescription, &skyDepthState);

	MFStartup(MF_VERSION);

	fileRenderData.filePath = L"C:\\Users\\Corey Rigney\\Videos\\Captures\\test103.mp4";
	fileRenderData.VideoBitRate = 800000;
	fileRenderData.VideoEncodingFormat = MFVideoFormat_H264;
	fileRenderData.VideoFPS = 30;
	fileRenderData.VideoFrameCount = 20 * fileRenderData.VideoFPS;
	fileRenderData.VideoFrameDuration = 10 * 1000 * 1000 / fileRenderData.VideoFPS;
	fileRenderData.VideoHeight = windowHeight;
	fileRenderData.VideoWidth = windowWidth;
	fileRenderData.VideoInputFormat = MFVideoFormat_RGB32;
	fileRenderData.VideoPels = fileRenderData.VideoWidth * fileRenderData.VideoHeight;

	PostResize(windowHeight, windowWidth, backBufferRTV, depthBufferDSV);
}

Renderer::~Renderer() {
	shadowDSVArray.clear();
	shadowProjMatArray.clear();
	shadowViewMatArray.clear();

	MFShutdown();
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

	finalCompositeTexture.Reset();
	renderTargetRTVs[RTVTypes::FINAL_COMPOSITE].Reset();
	renderTargetSRVs[RTVTypes::FINAL_COMPOSITE].Reset();

	device->CreateTexture2D(&compositeTexDesc, 0, finalCompositeTexture.GetAddressOf());
	device->CreateRenderTargetView(finalCompositeTexture.Get(), &compositeRTVDesc, renderTargetRTVs[RTVTypes::FINAL_COMPOSITE].GetAddressOf());

	device->CreateShaderResourceView(finalCompositeTexture.Get(), 0, renderTargetSRVs[RTVTypes::FINAL_COMPOSITE].GetAddressOf());

	fileWriteTexture.Reset();
	renderTargetSRVs[RTVTypes::FILE_WRITE_COMPOSITE].Reset();
	renderTargetRTVs[RTVTypes::FILE_WRITE_COMPOSITE].Reset();
	D3D11_TEXTURE2D_DESC fileTexDesc = {};
	fileTexDesc.Width = windowWidth;
	fileTexDesc.Height = windowHeight;
	fileTexDesc.ArraySize = 1;
	fileTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	fileTexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	fileTexDesc.MipLevels = 1;
	fileTexDesc.MiscFlags = 0;
	fileTexDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&fileTexDesc, 0, fileWriteTexture.GetAddressOf());

	D3D11_RENDER_TARGET_VIEW_DESC fileWriteRTVDesc = {};
	fileWriteRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	fileWriteRTVDesc.Texture2D.MipSlice = 0;
	fileWriteRTVDesc.Format = fileTexDesc.Format;

	device->CreateRenderTargetView(fileWriteTexture.Get(), &fileWriteRTVDesc, renderTargetRTVs[RTVTypes::FILE_WRITE_COMPOSITE].GetAddressOf());

	device->CreateShaderResourceView(fileWriteTexture.Get(), 0, renderTargetSRVs[RTVTypes::FILE_WRITE_COMPOSITE].GetAddressOf());

	fileReadTexture.Reset();
	D3D11_TEXTURE2D_DESC fileReadTexDesc = {};
	fileReadTexDesc.Width = windowWidth;
	fileReadTexDesc.Height = windowHeight;
	fileReadTexDesc.ArraySize = 1;
	fileReadTexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	fileReadTexDesc.MipLevels = 1;
	fileReadTexDesc.MiscFlags = 0;
	fileReadTexDesc.SampleDesc.Count = 1;
	fileReadTexDesc.Usage = D3D11_USAGE_STAGING;
	fileReadTexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	device->CreateTexture2D(&fileReadTexDesc, 0, fileReadTexture.GetAddressOf());

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
		XMVECTOR ssaoOffset = XMVector3Normalize(XMLoadFloat4(&ssaoOffsets[i]));

		float scale = (float)i / 64;
		XMVECTOR acceleratedScale = XMVectorLerp(
			XMVectorSet(0.1f, 0.1f, 0.1f, 1),
			XMVectorSet(1, 1, 1, 1),
			scale * scale
		);
		XMStoreFloat4(&ssaoOffsets[i], ssaoOffset * acceleratedScale);
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

	//Set up outlining buffers
	outlineTexture.Reset();
	outlineRTV.Reset();
	outlineSRV.Reset();

	D3D11_TEXTURE2D_DESC basicTexDesc = {};
	basicTexDesc.Width = windowWidth;
	basicTexDesc.Height = windowHeight;
	basicTexDesc.ArraySize = 1;
	basicTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	basicTexDesc.Format = DXGI_FORMAT_R32_FLOAT;
	basicTexDesc.MipLevels = 1;
	basicTexDesc.MiscFlags = 0;
	basicTexDesc.SampleDesc.Count = 1;
	device->CreateTexture2D(&basicTexDesc, 0, &outlineTexture);

	D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = 0;
	rtvDesc.Format = basicTexDesc.Format;
	device->CreateRenderTargetView(outlineTexture.Get(), &rtvDesc, outlineRTV.GetAddressOf());

	device->CreateShaderResourceView(outlineTexture.Get(), 0, outlineSRV.GetAddressOf());

	InitShadows();
}

void Renderer::InitShadows() {
	shadowCount = 0;
	shadowRasterizer.Reset();
	shadowSampler.Reset();
	this->VSShadow.reset();

	this->VSShadow = globalAssets.GetVertexShaderByName("ShadowVS");

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

	device->CreateDepthStencilView(silhouetteTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS].GetAddressOf());

	device->CreateDepthStencilView(transparentPrePassTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS].GetAddressOf());

	device->CreateDepthStencilView(prePassTexture, &shadowDSDesc, miscEffectDepthBuffers[MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS].GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(silhouetteTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS].GetAddressOf());

	device->CreateShaderResourceView(transparentPrePassTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::TRANSPARENT_PREPASS_DEPTHS].GetAddressOf());

	device->CreateShaderResourceView(prePassTexture, &srvDesc, miscEffectSRVs[MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS].GetAddressOf());

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
	shadowRastDesc.DepthBias = 1000;
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
void Renderer::DrawPointLights(std::shared_ptr<Camera> cam)
{
	// Turn on these shaders
	basicVS->SetShader();
	solidColorPS->SetShader();

	// Set up vertex shader
	basicVS->SetMatrix4x4("view", cam->GetViewMatrix());
	basicVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());

	context->IASetVertexBuffers(0, 1, sphereMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(sphereMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

	for (std::shared_ptr<Light> light : ComponentManager::GetAll<Light>())
	{
		// Only drawing points, so skip others
		if (light->GetType() != 1.0f || !light->IsEnabled())
			continue;

		// Calc quick scale based on range
		// (assuming range is between 5 - 10)
		float scale = light->GetRange() / 10.0f;

		// Set up the world matrix for this light
		basicVS->SetMatrix4x4("world", light->GetTransform()->GetWorldMatrix());

		// Set up the pixel shader data
		DirectX::XMFLOAT3 finalColor = light->GetColor();
		finalColor.x *= light->GetIntensity();
		finalColor.y *= light->GetIntensity();
		finalColor.z *= light->GetIntensity();
		solidColorPS->SetFloat3("Color", finalColor);

		// Copy data
		basicVS->CopyAllBufferData();
		solidColorPS->CopyAllBufferData();

		context->DrawIndexed(
			sphereMesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
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

	switch (type) {
	case MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS:
	{
		context->OMSetRenderTargets(1, renderTargetRTVs[RTVTypes::REFRACTION_SILHOUETTE].GetAddressOf(), depthBufferDSV.Get());

		context->OMSetDepthStencilState(refractionSilhouetteDepthState.Get(), 0);

		for (std::shared_ptr<MeshRenderer> mesh : ComponentManager::GetAll<MeshRenderer>()) {
			if (!mesh->IsEnabled() || !mesh->GetMaterial()->GetTransparent()) continue;

			// Standard depth pre-pass
			VSShadow->SetMatrix4x4("world", mesh->GetTransform()->GetWorldMatrix());

			VSShadow->CopyAllBufferData();

			solidColorPS->SetShader();
			solidColorPS->SetFloat3("Color", DirectX::XMFLOAT3(1, 1, 1));
			solidColorPS->CopyAllBufferData();

			context->IASetVertexBuffers(0, 1, mesh->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(mesh->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(
				mesh->GetMesh()->GetIndexCount(),
				0,
				0);
		}

		break;
	}

	case MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS:
	{
		context->OMSetRenderTargets(1, renderTargetRTVs[RTVTypes::DEPTHS].GetAddressOf(), depthBufferDSV.Get());

		for (std::shared_ptr<MeshRenderer> mesh : ComponentManager::GetAll<MeshRenderer>()) {
			if (!mesh->IsEnabled() || !mesh->GetMaterial()->GetTransparent()) continue;

			// Standard depth pre-pass
			VSShadow->SetMatrix4x4("world", mesh->GetTransform()->GetWorldMatrix());

			solidColorPS->SetShader();
			solidColorPS->SetFloat3("Color", DirectX::XMFLOAT3(1, 1, 1));
			solidColorPS->CopyAllBufferData();

			context->IASetVertexBuffers(0, 1, mesh->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(mesh->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(
				mesh->GetMesh()->GetIndexCount(),
				0,
				0);
		}

		break;
	}
	default:
		break;
	}

	context->OMSetDepthStencilState(0, 0);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	vp.Width = (float)windowWidth;
	vp.Height = (float)windowHeight;
	context->RSSetViewports(1, &vp);

	//context->RSSetState(0);
}

/// <summary>
/// Renders out the shadow maps for all active shadow projectors
/// </summary>
void Renderer::RenderShadows() {
	shadowDSVArray.clear();
	shadowProjMatArray.clear();
	shadowViewMatArray.clear();

	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	int newShadowCount = 0;
	//Renders each shadow map
	for (std::shared_ptr<Light> light : ComponentManager::GetAll<Light>()) {
		if (!light->IsEnabled() || !light->CastsShadows()) continue;

		std::shared_ptr<ShadowProjector> projector = light->GetShadowProjector();

		context->OMSetRenderTargets(0, 0, projector->GetDSV().Get());
		context->ClearDepthStencilView(projector->GetDSV().Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		context->RSSetState(shadowRasterizer.Get());

		vp.Width = (float)projector->GetProjectionWidth();
		vp.Height = (float)projector->GetProjectionHeight();
		context->RSSetViewports(1, &vp);

		VSShadow->SetShader();
		VSShadow->SetMatrix4x4("view", projector->GetViewMatrix());
		VSShadow->SetMatrix4x4("projection", projector->GetProjectionMatrix());
		VSShadow->CopyBufferData("perFrame");
		context->PSSetShader(0, 0, 0);

		for (std::shared_ptr<MeshRenderer> mesh : ComponentManager::GetAll<MeshRenderer>()) {
			//Ignores transparent meshes
			if (!mesh->IsEnabled() || mesh->GetMaterial()->GetTransparent()) continue;

			// This is similar to what I'd need for any depth pre-pass
			VSShadow->SetMatrix4x4("world", mesh->GetTransform()->GetWorldMatrix());
			VSShadow->CopyBufferData("perObject");

			context->IASetVertexBuffers(0, 1, mesh->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(mesh->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(
				mesh->GetMesh()->GetIndexCount(),
				0,
				0);
		}

		shadowDSVArray.emplace_back(projector->GetDSV());
		shadowProjMatArray.emplace_back(projector->GetProjectionMatrix());
		shadowViewMatArray.emplace_back(projector->GetViewMatrix());
		newShadowCount++;
	}

	//Regenerates the shadow array srv if the number of active maps changed
	if (shadowCount != newShadowCount) {
		shadowCount = newShadowCount;
		if (shadowCount > 0) {
			shadowDSVArraySRV.Reset();

			D3D11_TEXTURE2D_DESC shadowDesc = {};
			shadowDesc.Width = 2048; //Hard coded for simplicity, may need a more flexible solution later
			shadowDesc.Height = 2048; //Hard coded for simplicity, may need a more flexible solution later
			shadowDesc.ArraySize = shadowCount;
			shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
			shadowDesc.CPUAccessFlags = 0;
			shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			shadowDesc.MipLevels = 1;
			shadowDesc.MiscFlags = 0;
			shadowDesc.SampleDesc.Count = 1;
			shadowDesc.SampleDesc.Quality = 0;
			shadowDesc.Usage = D3D11_USAGE_DEFAULT;

			Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTexture;
			device->CreateTexture2D(&shadowDesc, 0, shadowTexture.GetAddressOf());

			D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
			shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
			shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
			shadowDSDesc.Texture2DArray.ArraySize = shadowCount;
			shadowDSDesc.Texture2DArray.FirstArraySlice = 0;
			shadowDSDesc.Texture2DArray.MipSlice = 0;
			device->CreateDepthStencilView(shadowTexture.Get(), &shadowDSDesc, &shadowDSVArray[0]);

			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.ArraySize = shadowCount;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;

			device->CreateShaderResourceView(shadowTexture.Get(), &srvDesc, shadowDSVArraySRV.GetAddressOf());
		}
	}

	//Copies the shadow maps to the array srv
	if (shadowCount > 0) {
		ID3D11Resource* arraySRVr = nullptr;
		shadowDSVArraySRV.Get()->GetResource(&arraySRVr);
		ID3D11Resource* shadowDSVr = nullptr;
		for (int i = 0; i < shadowCount; i++) {
			shadowDSVArray[i].Get()->GetResource(&shadowDSVr);
			context->CopySubresourceRegion(arraySRVr, i, 0, 0, 0, shadowDSVr, 0, 0);
		}
	}

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

	vp.Width = (float)windowWidth;
	vp.Height = (float)windowHeight;
	context->RSSetViewports(1, &vp);

	context->RSSetState(0);
}

void Renderer::RenderColliders(std::shared_ptr<Camera> cam)
{
	// Set the shaders to be used
	basicVS->SetShader();
	solidColorPS->SetShader();

	// Set up vertex shader
	basicVS->SetMatrix4x4("view", cam->GetViewMatrix());
	basicVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());

	//Draw in wireframe mode
	context->RSSetState(wireframeRasterizer.Get());

	context->IASetVertexBuffers(0, 1, cubeMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(cubeMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

	for (std::shared_ptr<Collider> collider : ComponentManager::GetAll<Collider>())
	{
		if (collider->IsEnabled() && collider->IsVisible()) {
			basicVS->SetMatrix4x4("world", collider->GetWorldMatrix());

			// Set up the pixel shader data
			XMFLOAT3 finalColor = XMFLOAT3(0.5f, 1.0f, 1.0f);
			// Drawing colliders and triggerboxes as different colors
			if (collider->IsTrigger())
			{
				finalColor = XMFLOAT3(1.0f, 1.0f, 0.0f);
			}
			solidColorPS->SetFloat3("Color", finalColor);

			// Copy data
			basicVS->CopyAllBufferData();
			solidColorPS->CopyAllBufferData();

			// Draw
			context->DrawIndexed(
				cubeMesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
				0,     // Offset to the first index we want to use
				0);    // Offset to add to each index when looking up vertices
		}
	}

	// Put the RS State back to normal (/non wireframe)
	context->RSSetState(0);
}

void Renderer::RenderMeshBounds(std::shared_ptr<Camera> cam)
{
	// Set the shaders to be used
	basicVS->SetShader();
	solidColorPS->SetShader();

	// Set up vertex shader
	basicVS->SetMatrix4x4("view", cam->GetViewMatrix());
	basicVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());

	//Draw in wireframe mode
	context->RSSetState(wireframeRasterizer.Get());

	context->IASetVertexBuffers(0, 1, cubeMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(cubeMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

	for (std::shared_ptr<MeshRenderer> mesh : ComponentManager::GetAll<MeshRenderer>())
	{
		if (mesh->IsEnabled() && mesh->DrawBounds) {
			//Make the world matrix for this bounds box
			BoundingOrientedBox obb = mesh->GetBounds();
			XMMATRIX transMat = XMMatrixTranslation(obb.Center.x, obb.Center.y, obb.Center.z);
			XMMATRIX scaleMat = XMMatrixScaling(obb.Extents.x * 2, obb.Extents.y * 2, obb.Extents.z * 2);
			XMMATRIX rotMat = XMMatrixRotationQuaternion(XMLoadFloat4(&obb.Orientation));

			XMFLOAT4X4 world;
			XMStoreFloat4x4(&world, scaleMat * rotMat * transMat);

			basicVS->SetMatrix4x4("world", world);

			// Set up the pixel shader data
			solidColorPS->SetFloat3("Color", XMFLOAT3(0.5f, 1.0f, 0.5f));

			// Copy data
			basicVS->CopyAllBufferData();
			solidColorPS->CopyAllBufferData();

			context->DrawIndexed(
				cubeMesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
				0,     // Offset to the first index we want to use
				0);    // Offset to add to each index when looking up vertices
		}
	}

	// Put the RS State back to normal (/non wireframe)
	context->RSSetState(0);
}

void Renderer::RenderSelectedHighlight(std::shared_ptr<Camera> cam, EngineState engineState)
{
	bool hasSelected = false;
	if (engineState == EngineState::EDITING && selectedEntity != -1) {
		context->RSSetState(0);
		context->OMSetDepthStencilState(0, 0);

		VSShadow->SetShader();
		VSShadow->SetMatrix4x4("view", cam->GetViewMatrix());
		VSShadow->SetMatrix4x4("projection", cam->GetProjectionMatrix());
		VSShadow->SetMatrix4x4("world", globalAssets.GetGameEntityAtID(selectedEntity)->GetTransform()->GetWorldMatrix());
		VSShadow->CopyAllBufferData();

		solidColorPS->SetShader();
		solidColorPS->SetFloat3("Color", XMFLOAT3(1.0f, 1.0f, 1.0f));
		solidColorPS->CopyAllBufferData();

		context->OMSetRenderTargets(1, outlineRTV.GetAddressOf(), 0);

		for (std::shared_ptr<MeshRenderer> mesh : globalAssets.GetGameEntityAtID(selectedEntity)->GetComponents<MeshRenderer>())
		{
			if (mesh->IsEnabled()) {
				context->IASetVertexBuffers(0, 1, mesh->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
				context->IASetIndexBuffer(mesh->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

				context->DrawIndexed(
					mesh->GetMesh()->GetIndexCount(),
					0,
					0);

				hasSelected = true;
			}
		}

		context->IASetVertexBuffers(0, 1, sphereMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(sphereMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		for (std::shared_ptr<ParticleSystem> particleSystem : globalAssets.GetGameEntityAtID(selectedEntity)->GetComponents<ParticleSystem>()) {
			if (particleSystem->IsEnabled()) {
				context->DrawIndexed(
					sphereMesh->GetIndexCount(),
					0,
					0);

				hasSelected = true;
			}
		}

		for (std::shared_ptr<Light> light : globalAssets.GetGameEntityAtID(selectedEntity)->GetComponents<Light>()) {
			if (light->IsEnabled()) {
				context->DrawIndexed(
					sphereMesh->GetIndexCount(),
					0,
					0);

				hasSelected = true;
			}
		}
	}

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);

	fullscreenVS->SetShader();

	outlinePS->SetShader();
	outlinePS->SetShaderResourceView("backBuffer", renderTargetSRVs[RTVTypes::FINAL_COMPOSITE].Get());
	outlinePS->SetShaderResourceView("FilledMeshTexture", outlineSRV.Get());
	outlinePS->SetSamplerState("samplerOptions", globalAssets.GetMaterialAtID(0)->GetClampSamplerState().Get());
	outlinePS->SetFloat3("borderColor", XMFLOAT3(1, 0, 1));
	outlinePS->SetFloat("pixelWidth", 1.0f / windowWidth);
	outlinePS->SetFloat("pixelHeight", 1.0f / windowHeight);
	outlinePS->SetInt("borderWidth", 1);
	outlinePS->SetInt("hasSelected", hasSelected);
	outlinePS->CopyAllBufferData();

	context->Draw(3, 0);

	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
}

bool Renderer::GetDrawColliderStatus() { return drawColliders; }
void Renderer::SetDrawColliderStatus(bool _newState) { drawColliders = _newState; }

void Renderer::Draw(std::shared_ptr<Camera> cam, EngineState engineState) {
	RenderShadows();

	// Background color (Black) for clearing
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

	context->ClearRenderTargetView(outlineRTV.Get(), color);

	// This fills in all renderTargets used before post-processing
	// Includes Colors, Ambient, Normals, and Depths
	const int numTargets = 4;
	static ID3D11RenderTargetView* renderTargets[numTargets] = {};
	for (int i = 0; i < numTargets; i++) {
		renderTargets[i] = renderTargetRTVs[i].Get();
	}
	context->OMSetRenderTargets(4, renderTargets, depthBufferDSV.Get());

	// Change to write depths beforehand - for future
	//RenderDepths(cam, MiscEffectSRVTypes::RENDER_PREPASS_DEPTHS);

	 //context->OMSetDepthStencilState(prePassDepthState.Get(), 0);

	// Per Frame data can be set out here for optimization
	// This section could be improved, see Chris's Demos and
	// Structs in header. Currently only supports the single default 
	// PBR+IBL shader
	unsigned int lightCount = Light::GetLightArrayCount();

	perFrameVS->SetMatrix4x4("view", cam->GetViewMatrix());
	perFrameVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	if (shadowCount > 0) {
		perFrameVS->SetData("shadowViews", shadowViewMatArray.data(), sizeof(XMFLOAT4X4) * MAX_LIGHTS);
		perFrameVS->SetData("shadowProjections", shadowProjMatArray.data(), sizeof(XMFLOAT4X4) * MAX_LIGHTS);
	}
	perFrameVS->SetInt("shadowCount", shadowCount);

	perFramePS->SetData("lights", Light::GetLightArray(), sizeof(LightData) * MAX_LIGHTS);
	perFramePS->SetData("lightCount", &lightCount, sizeof(unsigned int));
	perFramePS->SetFloat3("cameraPos", cam->GetTransform()->GetLocalPosition());
	if (globalAssets.currentSky->IsEnabled()) {
		perFramePS->SetInt("specIBLTotalMipLevels", globalAssets.currentSky->GetIBLMipLevelCount());
	}

	int meshIt = 0;

	// Material-Sort Rendering:
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

	std::vector<std::shared_ptr<MeshRenderer>> activeMeshes = ComponentManager::GetAll<MeshRenderer>();

	for (meshIt = 0; meshIt < activeMeshes.size() && !activeMeshes[meshIt]->GetMaterial()->GetTransparent(); meshIt++)
	{
		if (!activeMeshes[meshIt]->IsEnabled()) continue;

		//If the material needs to be swapped
		if (activeMeshes[meshIt]->GetMaterial().get() != currentMaterial)
		{
			// Eventual improvement:
			// Move all VS and PS "Set" calls into Material
			// This would require storing a lot more data in material
			// With shadows, it would also require passing in a lot of data
			// And handling edge cases like main camera swaps

			currentMaterial = activeMeshes[meshIt]->GetMaterial().get();

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
			currentPS->SetFloat("uvMult", currentMaterial->GetTiling());

			currentPS->CopyBufferData("PerMaterial");

			// Set textures and samplers
			currentPS->SetSamplerState("sampleState", currentMaterial->GetSamplerState().Get());
			currentPS->SetSamplerState("clampSampler", currentMaterial->GetClampSamplerState().Get());
			currentPS->SetShaderResourceView("textureAlbedo", currentMaterial->GetTexture()->GetTexture().Get());
			currentPS->SetShaderResourceView("textureRough", currentMaterial->GetRoughMap()->GetTexture().Get());
			currentPS->SetShaderResourceView("textureMetal", currentMaterial->GetMetalMap()->GetTexture().Get());
			if (currentMaterial->GetNormalMap() != nullptr) {
				currentPS->SetShaderResourceView("textureNormal", currentMaterial->GetNormalMap()->GetTexture().Get());
			}

			if (shadowCount > 0) {
				currentPS->SetShaderResourceView("shadowMaps", shadowDSVArraySRV.Get());
				currentPS->SetSamplerState("shadowState", shadowSampler.Get());
			}

			if (globalAssets.currentSky->IsEnabled()) {
				currentPS->SetShaderResourceView("irradianceIBLMap", globalAssets.currentSky->GetIrradianceCubeMap().Get());
				currentPS->SetShaderResourceView("brdfLookUpMap", globalAssets.currentSky->GetBRDFLookupTexture().Get());
				currentPS->SetShaderResourceView("specularIBLMap", globalAssets.currentSky->GetConvolvedSpecularCubeMap().Get());
			}
		}

		if (currentMesh != activeMeshes[meshIt]->GetMesh().get()) {
			currentMesh = activeMeshes[meshIt]->GetMesh().get();

			context->IASetVertexBuffers(0, 1, currentMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(currentMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);
		}

		if (currentVS != 0) {
			// Per-Object data
			currentVS->SetMatrix4x4("world", activeMeshes[meshIt]->GetTransform()->GetWorldMatrix());

			currentVS->CopyBufferData("PerObject");
		}

		if (currentMesh != 0) {
			context->DrawIndexed(currentMesh->GetIndexCount(), 0, 0);
		}
	}


	//Now deal with rendering the terrain, PS data first
	std::vector<std::shared_ptr<Terrain>> terrains = ComponentManager::GetAll<Terrain>();
	for (int i = 0; i < terrains.size(); i++) {
		if (!terrains[i]->IsEnabled()) continue;

		std::shared_ptr<TerrainMaterial> terrainMat = terrains[i]->GetMaterial();
		std::shared_ptr<SimplePixelShader> PSTerrain = terrains[i]->GetMaterial()->GetPixelShader();
		std::shared_ptr<SimpleVertexShader> VSTerrain = terrains[i]->GetMaterial()->GetVertexShader();

		PSTerrain->SetShader();
		PSTerrain->SetData("lights", Light::GetLightArray(), sizeof(Light) * MAX_LIGHTS);
		PSTerrain->SetData("lightCount", &lightCount, sizeof(unsigned int));
		PSTerrain->SetFloat3("cameraPos", cam->GetTransform()->GetLocalPosition());
		PSTerrain->SetFloat("uvMultNear", 50.0f);
		PSTerrain->SetFloat("uvMultFar", 150.0f);
		if (shadowCount > 0) {
			PSTerrain->SetShaderResourceView("shadowMaps", shadowDSVArraySRV.Get());
			PSTerrain->SetSamplerState("shadowState", shadowSampler.Get());
		}
		PSTerrain->SetShaderResourceView("blendMap", terrainMat->GetBlendMap().Get());
		PSTerrain->SetSamplerState("clampSampler", terrainMat->GetMaterialAtID(0)->GetClampSamplerState().Get());

		for (int i = 0; i < terrainMat->GetMaterialCount(); i++) {
			std::string a = "texture" + std::to_string(i + 1) + "Albedo";
			std::string n = "texture" + std::to_string(i + 1) + "Normal";
			std::string r = "texture" + std::to_string(i + 1) + "Rough";
			std::string m = "texture" + std::to_string(i + 1) + "Metal";
			PSTerrain->SetShaderResourceView(a, terrainMat->GetMaterialAtID(i)->GetTexture()->GetTexture().Get());
			PSTerrain->SetShaderResourceView(n, terrainMat->GetMaterialAtID(i)->GetNormalMap()->GetTexture().Get());
			PSTerrain->SetShaderResourceView(r, terrainMat->GetMaterialAtID(i)->GetRoughMap()->GetTexture().Get());
			PSTerrain->SetShaderResourceView(m, terrainMat->GetMaterialAtID(i)->GetMetalMap()->GetTexture().Get());
		}

		if (globalAssets.currentSky->IsEnabled()) {
			PSTerrain->SetInt("specIBLTotalMipLevels", globalAssets.currentSky->GetIBLMipLevelCount());
			PSTerrain->SetShaderResourceView("irradianceIBLMap", globalAssets.currentSky->GetIrradianceCubeMap().Get());
			PSTerrain->SetShaderResourceView("brdfLookUpMap", globalAssets.currentSky->GetBRDFLookupTexture().Get());
			PSTerrain->SetShaderResourceView("specularIBLMap", globalAssets.currentSky->GetConvolvedSpecularCubeMap().Get());
		}

		PSTerrain->CopyAllBufferData();

		VSTerrain->SetShader();

		VSTerrain->SetFloat4("colorTint", DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
		VSTerrain->SetMatrix4x4("world", terrains[i]->GetTransform()->GetWorldMatrix());
		VSTerrain->SetMatrix4x4("view", cam->GetViewMatrix());
		VSTerrain->SetMatrix4x4("projection", cam->GetProjectionMatrix());
		if (shadowCount > 0) {
			VSTerrain->SetData("shadowViews", shadowViewMatArray.data(), sizeof(XMFLOAT4X4) * MAX_LIGHTS);
			VSTerrain->SetData("shadowProjections", shadowProjMatArray.data(), sizeof(XMFLOAT4X4) * MAX_LIGHTS);
		}
		VSTerrain->SetInt("shadowCount", shadowCount);

		VSTerrain->CopyAllBufferData();

		context->IASetVertexBuffers(0, 1, terrains[i]->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(terrains[i]->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(
			terrains[i]->GetMesh()->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}

	if (globalAssets.currentSky->IsEnabled()) {
		context->RSSetState(skyRasterizer.Get());
		context->OMSetDepthStencilState(skyDepthState.Get(), 0);

		std::shared_ptr<SimplePixelShader> skyPixelShader = globalAssets.currentSky->GetPixShader();
		skyPixelShader->SetShader();
		skyPixelShader->SetSamplerState("sampleState", globalAssets.currentSky->GetSampler().Get());
		skyPixelShader->SetShaderResourceView("textureSky", globalAssets.currentSky->GetSkyTexture().Get());
		skyPixelShader->CopyAllBufferData();

		std::shared_ptr<SimpleVertexShader> skyVertexShader = globalAssets.currentSky->GetVertShader();
		skyVertexShader->SetShader();
		skyVertexShader->SetMatrix4x4("viewMat", cam->GetViewMatrix());
		skyVertexShader->SetMatrix4x4("projMat", cam->GetProjectionMatrix());
		skyVertexShader->CopyAllBufferData();

		context->IASetVertexBuffers(0, 1, cubeMesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(cubeMesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		context->DrawIndexed(cubeMesh->GetIndexCount(), 0, 0);

		context->RSSetState(0);
		context->OMSetDepthStencilState(0, 0);
	}

	fullscreenVS->SetShader();

	// SSAO Rendering
	// First step is the initial SSAO render pass
	renderTargets[0] = renderTargetRTVs[RTVTypes::SSAO_RAW].Get();
	renderTargets[1] = 0;
	renderTargets[2] = 0;
	renderTargets[3] = 0;
	context->OMSetRenderTargets(4, renderTargets, 0);

	ssaoPS->SetShader();

	// Inverse projection matrix
	XMFLOAT4X4 proj = cam->GetProjectionMatrix();
	XMFLOAT4X4 invProj;

	XMStoreFloat4x4(&invProj, XMMatrixInverse(0, XMLoadFloat4x4(&proj)));
	ssaoPS->SetMatrix4x4("invProjection", invProj);
	ssaoPS->SetMatrix4x4("projection", proj);
	ssaoPS->SetMatrix4x4("view", cam->GetViewMatrix());
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

	ssaoBlurPS->SetShader();
	ssaoBlurPS->SetShaderResourceView("SSAO", renderTargetSRVs[RTVTypes::SSAO_RAW].Get());
	ssaoBlurPS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ssaoBlurPS->CopyAllBufferData();
	context->Draw(3, 0);

	// Refractive and Transparent objects are drawn here
	// This uses refraction silhouette techniques, as well as
	// the depth pre-pass from earlier in Draw
	if (meshIt < activeMeshes.size())
	{
		renderTargets[0] = renderTargetRTVs[RTVTypes::COMPOSITE].Get();
	}
	else
	{
		renderTargets[0] = renderTargetRTVs[RTVTypes::FINAL_COMPOSITE].Get();
	}

	context->OMSetRenderTargets(1, renderTargets, 0);

	// Combine all results into the Composite buffer
	ssaoCombinePS->SetShader();
	ssaoCombinePS->SetShaderResourceView("sceneColorsNoAmbient", renderTargetSRVs[RTVTypes::COLORS_NO_AMBIENT].Get());
	ssaoCombinePS->SetShaderResourceView("ambient", renderTargetSRVs[RTVTypes::COLORS_AMBIENT].Get());
	ssaoCombinePS->SetShaderResourceView("SSAOBlur", renderTargetSRVs[RTVTypes::SSAO_BLUR].Get());
	//ssaoCombinePS->SetFloat2("pixelSize", XMFLOAT2(1.0f / windowWidth, 1.0f / windowHeight));
	ssaoCombinePS->CopyAllBufferData();
	context->Draw(3, 0);

	//Editing mode only debug renders
	if (engineState == EngineState::EDITING) {
		context->OMSetRenderTargets(1, renderTargets, depthBufferDSV.Get());
		if (drawColliders) RenderColliders(cam);
		RenderMeshBounds(cam);
		DrawPointLights(cam);
	}

	context->OMSetRenderTargets(1, renderTargets, (meshIt < activeMeshes.size()) ? depthBufferDSV.Get() : 0);

	//Render all of the emitters
	context->OMSetDepthStencilState(particleDepthState.Get(), 0);
	for (std::shared_ptr<ParticleSystem> emitter : ComponentManager::GetAll<ParticleSystem>()) 
	{
		if (emitter->IsEnabled())
			emitter->Draw(cam, particleBlendAdditive);
	}

	if (meshIt < activeMeshes.size())
	{
		context->OMSetBlendState(0, 0, 0xFFFFFFFF);

		fullscreenVS->SetShader();

		context->OMSetDepthStencilState(0, 0);

		// For the refraction merge, we need to store the composite
		// to a buffer that can be read by the GPU
		renderTargets[0] = renderTargetRTVs[RTVTypes::FINAL_COMPOSITE].Get();
		context->OMSetRenderTargets(1, renderTargets, 0);

		textureSamplePS->SetShader();
		textureSamplePS->SetShaderResourceView("Pixels", renderTargetSRVs[RTVTypes::COMPOSITE].Get());
		textureSamplePS->SetSamplerState("BasicSampler", activeMeshes[meshIt]->GetMaterial()->GetSamplerState());
		context->Draw(3, 0);

		// First, create the refraction silhouette
		RenderDepths(cam, MiscEffectSRVTypes::REFRACTION_SILHOUETTE_DEPTHS);

		// Then loop through and draw refractive objects
		renderTargets[0] = renderTargetRTVs[RTVTypes::FINAL_COMPOSITE].Get();
		context->OMSetRenderTargets(1, renderTargets, depthBufferDSV.Get());

		// Currently, all refractive shaders are the same, so this is fine
		std::shared_ptr<SimplePixelShader> refractivePS = activeMeshes[meshIt]->GetMaterial()->GetRefractivePixelShader();
		refractivePS->SetShader();

		refractivePS->SetFloat2("screenSize", XMFLOAT2((float)windowWidth, (float)windowHeight));
		refractivePS->SetMatrix4x4("viewMatrix", cam->GetViewMatrix());
		refractivePS->SetMatrix4x4("projMatrix", cam->GetProjectionMatrix());
		refractivePS->SetFloat3("cameraPos", cam->GetTransform()->GetLocalPosition());

		refractivePS->SetData("lights", Light::GetLightArray(), sizeof(Light) * MAX_LIGHTS);
		refractivePS->SetData("lightCount", &lightCount, sizeof(unsigned int));

		refractivePS->SetShaderResourceView("environmentMap", globalAssets.currentSky->GetSkyTexture().Get());

		refractivePS->SetShaderResourceView("screenPixels", renderTargetSRVs[RTVTypes::COMPOSITE].Get());
		refractivePS->SetShaderResourceView("refractionSilhouette", renderTargetSRVs[RTVTypes::REFRACTION_SILHOUETTE].Get());

		refractivePS->CopyBufferData("PerFrame");

		std::shared_ptr<SimpleVertexShader> refractiveVS = activeMeshes[meshIt]->GetMaterial()->GetVertShader();

		refractiveVS->SetShader();

		refractiveVS->SetMatrix4x4("view", cam->GetViewMatrix());
		refractiveVS->SetMatrix4x4("projection", cam->GetProjectionMatrix());
		refractiveVS->SetInt("shadowCount", 0);

		refractiveVS->CopyBufferData("PerFrame");

		for (meshIt = meshIt; meshIt < activeMeshes.size(); meshIt++) {
			if (!activeMeshes[meshIt]->IsEnabled()) continue;

			refractiveVS->SetFloat4("colorTint", activeMeshes[meshIt]->GetMaterial()->GetTint());

			refractiveVS->CopyBufferData("PerMaterial");

			refractiveVS->SetMatrix4x4("world", activeMeshes[meshIt]->GetTransform()->GetWorldMatrix());

			refractiveVS->CopyBufferData("PerObject");

			refractivePS->SetFloat("uvMult", activeMeshes[meshIt]->GetMaterial()->GetTiling());
			refractivePS->SetFloat("indexOfRefraction", activeMeshes[meshIt]->GetMaterial()->GetIndexOfRefraction());
			refractivePS->SetFloat("refractionScale", activeMeshes[meshIt]->GetMaterial()->GetRefractionScale());
			refractivePS->SetFloat("isRefractive", activeMeshes[meshIt]->GetMaterial()->GetRefractive());

			refractivePS->CopyBufferData("PerMaterial");

			refractivePS->SetShaderResourceView("textureNormal", activeMeshes[meshIt]->GetMaterial()->GetNormalMap()->GetTexture().Get());
			refractivePS->SetShaderResourceView("textureRoughness", activeMeshes[meshIt]->GetMaterial()->GetRoughMap()->GetTexture().Get());
			refractivePS->SetShaderResourceView("textureMetal", activeMeshes[meshIt]->GetMaterial()->GetMetalMap()->GetTexture().Get());

			context->IASetVertexBuffers(0, 1, activeMeshes[meshIt]->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
			context->IASetIndexBuffer(activeMeshes[meshIt]->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

			context->DrawIndexed(activeMeshes[meshIt]->GetMesh()->GetIndexCount(), 0, 0);
		}
	}

	RenderSelectedHighlight(cam, engineState);

	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
	context->OMSetDepthStencilState(0, 0);

	if (engineState == EngineState::FILE_RENDER) {
		// Don't present the screen to the user, and copy the final composite
		// to the CPU-Accessible fileRender buffer. Also convert it to an ARGB
		// format instead of RGBA

		renderTargets[0] = renderTargetRTVs[RTVTypes::FILE_WRITE_COMPOSITE].Get();
		context->OMSetRenderTargets(1, renderTargets, 0);

		std::shared_ptr<SimplePixelShader> rgbConvertPS = globalAssets.GetPixelShaderByName("CompressRGBPS");

		fullscreenVS->SetShader();

		rgbConvertPS->SetShader();
		rgbConvertPS->SetShaderResourceView("RGBFrame", renderTargetSRVs[RTVTypes::FINAL_COMPOSITE].Get());
		rgbConvertPS->SetSamplerState("BasicSampler", globalAssets.GetMaterialAtID(0)->GetSamplerState());
		rgbConvertPS->CopyAllBufferData();
		context->Draw(3, 0);

		context->CopyResource(fileReadTexture.Get(), fileWriteTexture.Get());

		// Present the back buffer to the user
		//  - Puts the final frame we're drawing into the window so the user can see it
		//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
		swapChain->Present(0, 0);

		// Due to the usage of a more sophisticated swap chain,
		// the render target must be re-bound after every call to Present()
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());

		//ID3D11Resource* finalCompositeResource;
		//context->CopyResource(fileWriteTexture.Get(), finalCompositeTexture.Get());

		//context->CopyResource(readableRenderComposite.Get(), finalCompositeResource);
	}
	else {
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		// Present the back buffer to the user
		//  - Puts the final frame we're drawing into the window so the user can see it
		//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
		swapChain->Present(0, 0);

		// Due to the usage of a more sophisticated swap chain,
		// the render target must be re-bound after every call to Present()
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}

	// Unbind all in-use shader resources
	ID3D11ShaderResourceView* nullSRVs[32] = {};
	context->PSSetShaderResources(0, 32, nullSRVs);
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetRenderTargetSRV(RTVTypes type) {
	return renderTargetSRVs[type];
}

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Renderer::GetMiscEffectSRV(MiscEffectSRVTypes type) {
	return miscEffectSRVs[type];
}

HRESULT Renderer::RenderToVideoFile(std::shared_ptr<Camera> renderCam, FileRenderData RenderParameters) {
	// Process of this:
	// Call draw to get final composite frame. (possibly prevent it presenting to the screen?)
	// Have CPU accessible buffer of same type as final composite.
	// Copy final composite into this buffer.
	// Access buffer's data to get array of pixel colors.
	// Have media sink initialized (or maybe initialize in here? Could be laggy on first boot/scene load otherwise)
	// Use media sink tutorial on writing to video files
	// Make sure data is locked and released correctly, seems potentially unstable
	// May need to convert final composite data to YUV encoding, since RGBA is super uncompressed (high quality tho)
	// This method will needs either lots of parameters or lots of global data. Some stuff like width/height can be pulled from engine data.

	Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter = NULL;
	DWORD streamIndex;
	long long int timeStamp = 0;

	RETURN_HRESULT_IF_FAILED(InitializeFileSinkWriter(&sinkWriter, &streamIndex, &RenderParameters));

	for (DWORD i = 0; i < RenderParameters.VideoFrameCount; ++i)
	{
		// Call draw in to generate the desired frame. Instead of presenting to the screen,
		// Draw will copy the final composite to a buffer (given the FILE_RENDER State.)
		Draw(renderCam, EngineState::FILE_RENDER);

		RETURN_HRESULT_IF_FAILED(WriteFrame(sinkWriter, streamIndex, timeStamp, &RenderParameters));

		timeStamp += RenderParameters.VideoFrameDuration;
	}

	sinkWriter->Finalize();

	sinkWriter->Release();

	return 0;
}

HRESULT Renderer::InitializeFileSinkWriter(Microsoft::WRL::ComPtr<IMFSinkWriter>* sinkWriterOut, DWORD* pStreamIndex, FileRenderData* RenderParameters) {
	*pStreamIndex = NULL;

	Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter = NULL;
	Microsoft::WRL::ComPtr<IMFMediaType> pMediaTypeOut = NULL;
	Microsoft::WRL::ComPtr<IMFMediaType> pMediaTypeIn = NULL;
	DWORD streamIndex;

	RETURN_HRESULT_IF_FAILED(MFCreateDXGIDeviceManager(&deviceManagerResetToken, &deviceManager));
	RETURN_HRESULT_IF_FAILED(deviceManager->ResetDevice(device.Get(), deviceManagerResetToken));

	RETURN_HRESULT_IF_FAILED(MFCreateSinkWriterFromURL(RenderParameters->filePath.c_str(), NULL, NULL, &sinkWriter));

	// Set the output media type.
	RETURN_HRESULT_IF_FAILED(MFCreateMediaType(&pMediaTypeOut));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, RenderParameters->VideoEncodingFormat));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, RenderParameters->VideoBitRate));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));

	RETURN_HRESULT_IF_FAILED(MFSetAttributeSize(pMediaTypeOut.Get(), MF_MT_FRAME_SIZE, RenderParameters->VideoWidth, RenderParameters->VideoHeight));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_FRAME_RATE, RenderParameters->VideoFPS, 1));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeOut.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	
	RETURN_HRESULT_IF_FAILED(sinkWriter->AddStream(pMediaTypeOut.Get(), &streamIndex));
	
	// Set the input media type. May need to change to accept graphics buffer?
	RETURN_HRESULT_IF_FAILED(MFCreateMediaType(&pMediaTypeIn));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, RenderParameters->VideoInputFormat));
	
	RETURN_HRESULT_IF_FAILED(pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeSize(pMediaTypeIn.Get(), MF_MT_FRAME_SIZE, RenderParameters->VideoWidth, RenderParameters->VideoHeight));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_FRAME_RATE, RenderParameters->VideoFPS, 1));
	
	RETURN_HRESULT_IF_FAILED(MFSetAttributeRatio(pMediaTypeIn.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	
	RETURN_HRESULT_IF_FAILED(sinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn.Get(), NULL));
	
	// Tell the sink writer to start accepting data.
	RETURN_HRESULT_IF_FAILED(sinkWriter->BeginWriting());
	
	// Return the pointer to the caller.
	*sinkWriterOut = sinkWriter;
	(*sinkWriterOut)->AddRef();
	*pStreamIndex = streamIndex;

	/*sinkWriter->Release();
	pMediaTypeOut->Release();
	pMediaTypeIn->Release();*/
	return 0;
}

HRESULT Renderer::WriteFrame(Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter, DWORD streamIndex, const long long int& timeStamp, FileRenderData* RenderParameters) {
	Microsoft::WRL::ComPtr<IMFSample> pSample = NULL;
	Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer = NULL;

	D3D11_MAPPED_SUBRESOURCE msr;
	ZeroMemory(&msr, sizeof(D3D11_MAPPED_SUBRESOURCE));

	const LONG cbWidth = 4 * RenderParameters->VideoWidth;
	const DWORD cbBuffer = cbWidth * RenderParameters->VideoHeight;

	BYTE* pData = NULL;

	// Create a new memory buffer.
	HRESULT hr = MFCreateMemoryBuffer(cbBuffer, &pBuffer);

	// Lock the buffer and copy the video frame to the buffer.
	RETURN_HRESULT_IF_FAILED(pBuffer->Lock(&pData, NULL, NULL));

	context->Map(fileReadTexture.Get(), 0, D3D11_MAP_READ, 0, &msr);

	RETURN_HRESULT_IF_FAILED(MFCopyImage(
			pData,							// Destination buffer.
			cbWidth,						// Destination stride.
			(BYTE*)(msr.pData) + ((RenderParameters->VideoHeight - 1) * cbWidth),		// First row in rendered buffer. maybe use the resource with CPU access and a CopyResource??
			-cbWidth,						// Source stride
			cbWidth,						// Image width in bytes.
			RenderParameters->VideoHeight   // Image height in pixels.
		));

	context->Unmap(fileReadTexture.Get(), 0);

	if (pBuffer)
	{
		pBuffer->Unlock();
	}

	// Set the data length of the buffer.
	RETURN_HRESULT_IF_FAILED(pBuffer->SetCurrentLength(cbBuffer));

	// Create a media sample and add the buffer to the sample.
	RETURN_HRESULT_IF_FAILED(MFCreateSample(&pSample));

	RETURN_HRESULT_IF_FAILED(pSample->AddBuffer(pBuffer.Get()));

	// Set the time stamp and the duration.
	RETURN_HRESULT_IF_FAILED(pSample->SetSampleTime(timeStamp));
	
	RETURN_HRESULT_IF_FAILED(pSample->SetSampleDuration(RenderParameters->VideoFrameDuration));

	// Send the sample to the Sink Writer.
	RETURN_HRESULT_IF_FAILED(sinkWriter->WriteSample(streamIndex, pSample.Get()));

	/*pSample->Release();
	pBuffer->Release();*/
	return hr;
}

FileRenderData* Renderer::GetFileRenderData() {
	return &fileRenderData;
}