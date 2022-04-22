#pragma once

#include "Material.h"
#include "SpriteFont.h"

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

// Basic particle struct
struct Particle 
{
	float age;
	DirectX::XMFLOAT3 startPos;
	float emitTime;
	DirectX::XMFLOAT3 currentPos;
	float alive;
	DirectX::XMFLOAT3 padding;
};

// SHOE Fonts need a filekey for saving/loading
struct SHOEFont
{
	std::string fileNameKey;
	std::string name;
	std::shared_ptr<DirectX::SpriteFont> spritefont;
};

enum ParticleComputeShaderType
{
	Emit,
	Simulate,
	Copy,
	DeadListInit
};