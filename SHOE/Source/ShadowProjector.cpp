#include "..\Headers\ShadowProjector.h"
#include "..\Headers\AssetManager.h"
#include "..\Headers\Light.h"
#include <DirectXMath.h>

using namespace DirectX;

void ShadowProjector::BindLight(std::shared_ptr<Light> light)
{
	boundLight = light;
	Camera::Start();
	SetProjectionDimensions(2048, 2048);
}

void ShadowProjector::UpdateFieldsByLightType()
{
	if (IsEnabled()) {
		//Directional Light
		if (boundLight->GetType() == 0.0f) {
			SetProjectionMatrixType(false);
			SetFarDist(500.0f);
		}
		//Spot Light
		else if (boundLight->GetType() == 2.0f) {
			SetProjectionMatrixType(true);
			SetFarDist(std::max<float>(boundLight->GetRange(), GetNearDist() + 0.1f));
		}
	}
}

void ShadowProjector::UpdateViewMatrix()
{
	XMVECTOR position = XMLoadFloat3(&GetTransform()->GetGlobalPosition());

	DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&boundLight->GetDirection());
	DirectX::XMVECTOR rot = DirectX::XMLoadFloat4(&GetTransform()->GetGlobalRotation());

	XMMATRIX view = XMMatrixLookToLH(position, DirectX::XMVector3Rotate(dir, rot), XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&vMatrix, view);
}

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
	boundLight = nullptr;
}

void ShadowProjector::OnEnable()
{
	if (!boundLight->IsEnabled() || !boundLight->CastsShadows()) {
		SetEnabled(false);
	}
	else {
		UpdateFieldsByLightType();
	}
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
