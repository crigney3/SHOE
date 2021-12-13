#pragma once

#include "Material.h"

// --------------------------------------------------------
// A custom vertex definition
//
// You will eventually ADD TO this, and/or make more of these!
// --------------------------------------------------------
struct Vertex
{
	DirectX::XMFLOAT3 Position;	    // The position of the vertex
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT2 uv;
};

// Struct to hold Terrain data
struct TerrainMats
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> blendMap;
	std::vector<Material> blendMaterials;
	std::string name;
};

// Basic particle struct
struct Particle 
{
	float emitTime;
	DirectX::XMFLOAT3 startPos;
};

enum ParticleComputeShaderType
{
	Emit,
	Simulate,
	Copy
};