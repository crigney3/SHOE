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
	std::shared_ptr<SimpleVertexShader> vertShader;
	//float specularExponent;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap;
public:
	Material(DirectX::XMFLOAT4 tint,
		std::shared_ptr<SimplePixelShader> pix,
		std::shared_ptr<SimpleVertexShader> vert,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> texture,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> textureState,
		Microsoft::WRL::ComPtr<ID3D11SamplerState> clampState,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> roughMap,
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> metalMap);
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
};

