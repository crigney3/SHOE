#pragma once
#include "Camera.h"
#include <d3d11.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

//Not really enforced bc any computer would die before this is reached
#define MAX_SHADOW_PROJECTORS 32

class ShadowProjector : public Camera
{
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV();
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> GetDSV();

	int GetProjectionWidth();
	int GetProjectionHeight();
	void SetProjectionDimensions(int projectionWidth, int projectionHeight);
private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	ID3D11Texture2D* shadowTexture;

	int projectionWidth;
	int projectionHeight;

	void Start() override;
	void RegenerateResources();
};

