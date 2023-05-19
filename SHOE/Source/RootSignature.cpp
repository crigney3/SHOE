#include "../Headers/RootSignature.h"

RootSignature::RootSignature() {
	this->inputElementsList = std::vector<D3D12_INPUT_ELEMENT_DESC>();
	this->rootParamsList = std::vector<D3D12_ROOT_PARAMETER>();

	dxInstance = DXCore::DXCoreInstance;
}

RootSignature::~RootSignature() {

}

void RootSignature::LoadShaderFromName(std::wstring name, SimpleShaderType type) {
	Microsoft::WRL::ComPtr<ID3DBlob> outputBlob;
	std::wstring fullPath;

	fullPath = dxInstance->GetFullPathTo_Wide(name);

	if (type == SimpleDX12VertexShaderType) {
		D3DReadFileToBlob(fullPath.c_str(), vertexShaderByteCode.GetAddressOf());
	} 
	else if (type == SimpleDX12PixelShaderType) {
		D3DReadFileToBlob(fullPath.c_str(), pixelShaderByteCode.GetAddressOf());
	}
	else if (type == SimpleDX12ComputeShaderType) {
		D3DReadFileToBlob(fullPath.c_str(), computeShaderByteCode.GetAddressOf());
	}
}

void RootSignature::AppendInputElement(DXGI_FORMAT format, std::string semanticName, int semanticIndex, ULONG byteOffset) {
	D3D12_INPUT_ELEMENT_DESC tempDesc;

	tempDesc.AlignedByteOffset = byteOffset;
	tempDesc.Format = format;
	tempDesc.SemanticName = semanticName.c_str();
	tempDesc.SemanticIndex = semanticIndex;

	this->inputElementsList.push_back(tempDesc);
}

void RootSignature::CreateCBVTable(D3D12_DESCRIPTOR_RANGE cbvRange, D3D12_SHADER_VISIBILITY shaderVisibility) {
	D3D12_ROOT_PARAMETER tempParam;

	tempParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	tempParam.ShaderVisibility = shaderVisibility;
	tempParam.DescriptorTable.NumDescriptorRanges = 1;
	tempParam.DescriptorTable.pDescriptorRanges = &cbvRange;

	this->rootParamsList.push_back(tempParam);
}

void RootSignature::CreateSRVTable(D3D12_DESCRIPTOR_RANGE srvRange, D3D12_SHADER_VISIBILITY shaderVisibility) {
	D3D12_ROOT_PARAMETER tempParam;

	tempParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	tempParam.ShaderVisibility = shaderVisibility;
	tempParam.DescriptorTable.NumDescriptorRanges = 1;
	tempParam.DescriptorTable.pDescriptorRanges = &srvRange;

	this->rootParamsList.push_back(tempParam);
}

void RootSignature::SetSamplerState(D3D12_STATIC_SAMPLER_DESC sampler) {
	this->samplerList.push_back(sampler);
}

bool RootSignature::InitializeRootSignature(Microsoft::WRL::ComPtr<ID3D12Device> device) {
	int rootParamSize = this->rootParamsList.size();
	int samplersSize = this->samplerList.size();

	D3D12_ROOT_PARAMETER* rootParams = new D3D12_ROOT_PARAMETER[rootParamSize];
	D3D12_STATIC_SAMPLER_DESC* samplers = new D3D12_STATIC_SAMPLER_DESC[samplersSize];

	for (int i = 0; i < rootParamSize; i++) {
		rootParams[i] = this->rootParamsList[i];
	}

	D3D12_ROOT_SIGNATURE_DESC rootSig = {};
	rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSig.NumParameters = rootParamSize;
	rootSig.pParameters = rootParams;
	rootSig.NumStaticSamplers = samplersSize;
	rootSig.pStaticSamplers = samplers;

	ID3DBlob* serializedRootSig = 0;
	ID3DBlob* errors = 0;

	D3D12SerializeRootSignature(
		&rootSig,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig,
		&errors);

	// Check for errors during serialization
	//if (errors != 0)
	//{
	//	OutputDebugString((wchar_t*)errors->GetBufferPointer());
	//}

	// Actually create the root sig
	device->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(internalRootSig.GetAddressOf()));

	delete[] rootParams;

	if (errors != 0) {
		return 0;
	}

	return 1;
}