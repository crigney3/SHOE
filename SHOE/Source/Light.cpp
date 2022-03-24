#include "..\Headers\Light.h"
#include "..\Headers\GameEntity.h"
#include "..\Headers\ComponentManager.h"

bool Light::lightArrayDirty = false;
std::vector<LightData> Light::lightData = std::vector<LightData>();

LightData Light::GetData()
{
	DirectX::XMVECTOR dir = DirectX::XMLoadFloat3(&direction);
	DirectX::XMVECTOR rot = DirectX::XMLoadFloat4(&GetGameEntity()->GetTransform()->GetGlobalRotation());
	DirectX::XMFLOAT3 finalFacing;
	DirectX::XMStoreFloat3(&finalFacing, DirectX::XMVector3Rotate(dir, rot));
	return LightData{
		type,
		color,
		intensity,
		finalFacing,
		(float)IsEnabled(),
		GetGameEntity()->GetTransform()->GetGlobalPosition(),
		range
	};
}

LightData* Light::GetLightArray()
{
	if (lightArrayDirty) {
		std::vector<std::shared_ptr<Light>> lights = ComponentManager::GetAll<Light>();
		lightData.clear();
		for (std::shared_ptr<Light> light : lights) {
			lightData.push_back(light->GetData());
		}
		lightArrayDirty = false;
	}
	return lightData.data();
}

int Light::GetLightArrayCount()
{
	return lightData.size();
}

void Light::MarkDirty()
{
	lightArrayDirty = true;
}

void Light::Start()
{
	type = 0.0f;
	color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	intensity = 1.0f;
	direction = DirectX::XMFLOAT3(1, 0, 0);
	range = 100.0f;

	lightArrayDirty = true;
}

void Light::OnDestroy()
{
	lightArrayDirty = true;
}

float Light::GetType()
{
	return type;
}

void Light::SetType(float type)
{
	if (this->type != type) {
		this->type = type;
		lightArrayDirty = true;
	}
}

DirectX::XMFLOAT3 Light::GetColor()
{
	return color;
}

void Light::SetColor(DirectX::XMFLOAT3 color)
{
	if (this->color.x != color.x || this->color.y != color.y || this->color.z != color.z) {
		this->color = color;
		lightArrayDirty = true;
	}
}

float Light::GetIntensity()
{
	return intensity;
}

void Light::SetIntensity(float intensity)
{
	if (this->intensity != intensity) {
		this->intensity;
		lightArrayDirty = true;
	}
}

DirectX::XMFLOAT3 Light::GetDirection()
{
	return direction;
}

void Light::SetDirection(DirectX::XMFLOAT3 direction)
{
	if (this->direction.x != direction.x || this->direction.y != direction.y || this->direction.z != direction.z) {
		this->direction = direction;
		lightArrayDirty = true;
	}
}

float Light::GetRange()
{
	return range;
}

void Light::SetRange(float range)
{
	if (this->range != range) {
		this->range = range;
		lightArrayDirty = true;
	}
}
