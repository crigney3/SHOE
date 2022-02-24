#pragma once

#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "DXCore.h"
#include "SimpleShader.h"
#include <memory>
#include <vector>

class Material
{
private:
	float uvTiling;
	DirectX::XMFLOAT4 colorTint;
	std::shared_ptr<SimplePixelShader> pixShader;
	std::shared_ptr<SimplePixelShader> refractivePixShader;
	std::shared_ptr<SimpleVertexShader> vertShader;
	//float specularExponent;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap;

	bool enabled;
	std::string name;

	bool transparent;
	bool refractive;

	float indexOfRefraction;
	float refractionScale;

public:
	Material(DirectX::XMFLOAT4 tint,
			 std::shared_ptr<SimplePixelShader> pix,
			 std::shared_ptr<SimpleVertexShader> vert,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
			 Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
			 Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap,
			 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap,
			 std::string name = "material",
			 bool transparent = false,
			 bool refractive = false);
	~Material();

	DirectX::XMFLOAT4 GetTint();
	void SetTint(DirectX::XMFLOAT4 tint);
	std::shared_ptr<SimplePixelShader> GetPixShader();
	std::shared_ptr<SimpleVertexShader> GetVertShader();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetNormalMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetMetalMap();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetRoughMap();
	//float GetSpecExponent();
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetTexture();
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetSamplerState();
	Microsoft::WRL::ComPtr<ID3D11SamplerState> GetClampSamplerState();
	float GetTiling();
	void SetTiling(float uv);

	std::string GetName();

	void SetTransparent(bool transparent);
	bool GetTransparent();

	void SetRefractive(bool refractive);
	bool GetRefractive();

	void SetIndexOfRefraction(float index);
	float GetIndexOfRefraction();

	void SetRefractionScale(float scale);
	float GetRefractionScale();

	void SetPixelShader(std::shared_ptr<SimplePixelShader> pix);
	void SetVertexShader(std::shared_ptr<SimpleVertexShader> vert);

	void SetRefractivePixelShader(std::shared_ptr<SimplePixelShader> refractPix);
	std::shared_ptr<SimplePixelShader> GetRefractivePixelShader();

	void SetEnableDisable(bool value);
	bool GetEnableDisable();
};

