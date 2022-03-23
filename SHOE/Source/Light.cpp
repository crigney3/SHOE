#include "..\Headers\Light.h"
#include "..\Headers\GameEntity.h"
#include "..\Headers\ComponentManager.h"

LightData Light::GetData()
{
	return LightData{
		type,
		color,
		intensity,
		direction,
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
