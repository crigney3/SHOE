#include "..\Headers\ShadowProjector.h"
#include "..\Headers\AssetManager.h"

Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ShadowProjector::GetSRV()
{
	return shadowSRV;
}

Microsoft::WRL::ComPtr<ID3D11DepthStencilView> ShadowProjector::GetDSV()
{
	return shadowDSV;
}

int ShadowProjector::GetProjectionWidth()
{
	return projectionWidth;
}

int ShadowProjector::GetProjectionHeight()
{
	return projectionHeight;
}

void ShadowProjector::SetProjectionDimensions(int projectionWidth, int projectionHeight)
{
	this->projectionWidth = projectionWidth;
	this->projectionHeight = projectionHeight;
	RegenerateResources();
}

void ShadowProjector::Start()
{
	Camera::Start();
	SetProjectionDimensions(2048, 2048);
}

void ShadowProjector::RegenerateResources()
{
	Microsoft::WRL::ComPtr<ID3D11Device> device = AssetManager::GetInstance().GetDevice();

	shadowDSV->Release();
	shadowSRV->Release();

	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = projectionWidth;
	shadowDesc.Height = projectionHeight;
	shadowDesc.ArraySize = 1;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.CPUAccessFlags = 0;
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;

	device->CreateTexture2D(&shadowDesc, 0, &shadowTexture);

	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDSDesc = {};
	shadowDSDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDSDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	shadowDSDesc.Texture2D.MipSlice = 0;
	device->CreateDepthStencilView(shadowTexture, &shadowDSDesc, shadowDSV.GetAddressOf());

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture, &srvDesc, shadowSRV.GetAddressOf());

	shadowTexture->Release();
}
