#pragma once

#include "DXCore.h"
#include <d3dcompiler.h>
#include <vector>

enum SimpleShaderType
{
	SimpleVertexShaderType,
	SimplePixelShaderType,
	SimpleComputeShaderType,
	SimpleDX12VertexShaderType,
	SimpleDX12PixelShaderType,
	SimpleDX12ComputeShaderType,
	MaxSimpleShaderTypeValue
};

// This class exists to help automate shaders that need
// to be run in DX12. It is a wrapper for the actual
// GPU root signature, and allows management of its resources.

class RootSignature
{
public:

	RootSignature();
	~RootSignature();

	void LoadShaderFromName(std::wstring name, SimpleShaderType type);

	void AppendInputElement(DXGI_FORMAT format, std::string semanticName, int semanticIndex = 0, ULONG byteOffset = D3D12_APPEND_ALIGNED_ELEMENT);

	void CreateCBVTable(D3D12_DESCRIPTOR_RANGE cbvRange, D3D12_SHADER_VISIBILITY shaderVisibility);
	void CreateSRVTable(D3D12_DESCRIPTOR_RANGE srvRange, D3D12_SHADER_VISIBILITY shaderVisibility);

	// Once each existing sampler state has been bound to a register once,
	// it doesn't need to be bound again.
	void SetSamplerState(D3D12_STATIC_SAMPLER_DESC sampler);

	// Only call this once everything is set correctly!
	bool InitializeRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> device);

	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementsList;
	std::vector<D3D12_ROOT_PARAMETER> rootParamsList;
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplerList;

private:

	DXCore* dxInstance;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> internalRootSig;

	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> computeShaderByteCode;

};