#pragma once
#include <DirectXMath.h>
#include "IComponent.h"
#include <vector>

struct LightData {
	float type;
	DirectX::XMFLOAT3 color;
	float intensity;
	DirectX::XMFLOAT3 direction;
	float enabled;
	DirectX::XMFLOAT3 position;
	float range;
	DirectX::XMFLOAT3 padding;
};

class Light : public IComponent
{
private:
	static bool lightArrayDirty;
	static std::vector<LightData> lightData;

	float type;
	DirectX::XMFLOAT3 color;
	float intensity;
	DirectX::XMFLOAT3 direction;
	float range;

	LightData GetData();
public:
	static LightData* GetLightArray();
	/// <summary>
	/// TODO: Set dirty in UpdateWorldInfo when GameEntity w/ light's transform changes
	/// TODO: Set dirty when light's IsEnabled changes
	/// </summary>
	static void MarkDirty();

	void Start() override;
	void OnDestroy() override;
};

