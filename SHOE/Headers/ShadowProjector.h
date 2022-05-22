#pragma once
#include "Camera.h"
#include <d3d11.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects

class Light;

class ShadowProjector : public Camera
{
public:
	void BindLight(std::shared_ptr<Light> light);
	void UpdateFieldsByLightType();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetSRV();
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> GetDSV();

	int GetProjectionWidth();
	int GetProjectionHeight();
private:
	std::shared_ptr<Light> boundLight;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	ID3D11Texture2D* shadowTexture;

	int projectionWidth;
	int projectionHeight;

	void Start() override;
	void OnEnable() override;
	void RegenerateResources();
	//Might be public later, but would make shader logic more complex
	void SetProjectionDimensions(int projectionWidth, int projectionHeight);
};

