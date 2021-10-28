#pragma once
#include <DirectXMath.h>

struct Light {
	float type;
	DirectX::XMFLOAT3 color;
	float intensity;
	DirectX::XMFLOAT3 direction;
	float enabled;
	DirectX::XMFLOAT3 position;
	float range;
	DirectX::XMFLOAT3 padding;
};

//These structs just exist to look nice in code
struct DirectionalLight : Light {

};

struct PointLight : Light {

};

struct SpotLight : Light {

};