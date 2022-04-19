#pragma once
#include "IComponent.h"
#include <vector>

#define MAX_LIGHTS 64

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
	static int GetLightArrayCount();

	void Start() override;
	void OnDestroy() override;
	void OnMove(DirectX::XMFLOAT3 delta) override;
	void OnRotate(DirectX::XMFLOAT3 delta) override;
	void OnEnabledChanged(bool newState) override;

	float GetType();
	void SetType(float type);
	DirectX::XMFLOAT3 GetColor();
	void SetColor(DirectX::XMFLOAT3 color);
	float GetIntensity();
	void SetIntensity(float intensity);
	DirectX::XMFLOAT3 GetDirection();
	void SetDirection(DirectX::XMFLOAT3 direction);
	float GetRange();
	void SetRange(float range);
};
