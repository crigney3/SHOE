#include "..\Headers\Light.h"
#include "..\Headers\GameEntity.h"
#include "..\Headers\ComponentManager.h"

bool Light::lightArrayDirty = false;
std::vector<LightData> Light::lightData = std::vector<LightData>();

/// <summary>
/// Gets the data from this light in LightData format
/// </summary>
/// <returns>A LightData populated with this frame's data</returns>
LightData Light::GetData()
{
	return LightData{
		type,
		color,
		intensity,
		GetTransform()->GetForward(),
		(float)IsEnabled(),
		GetTransform()->GetGlobalPosition(),
		range,
		(float)castsShadows
	};
}

/// <summary>
/// Updates and gets a reference to this frame's array of LightData
/// </summary>
/// <returns>A reference to the array</returns>
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

/// <summary>
/// Populates this light with default data
/// </summary>
void Light::Start()
{
	type = 0.0f;
	color = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	intensity = 1.0f;
	range = 100.0f;
	castsShadows = false;
	shadowProjector = nullptr;

	lightArrayDirty = true;
}

void Light::OnDestroy()
{
	lightArrayDirty = true;
	if (shadowProjector != nullptr) {
		shadowProjector->OnDestroy();
		ComponentManager::Free<ShadowProjector>(shadowProjector);
		shadowProjector = nullptr;
	}
}

void Light::MarkLightAndShadowsDirty() {
	lightArrayDirty = true;
	if (shadowProjector != nullptr)
		shadowProjector->UpdateViewMatrix();
}

void Light::OnTransform()
{
	MarkLightAndShadowsDirty();
}

void Light::OnParentTransform(std::shared_ptr<GameEntity> parent)
{
	MarkLightAndShadowsDirty();
}

void Light::OnParentMove(std::shared_ptr<GameEntity> parent) {
	MarkLightAndShadowsDirty();
}

void Light::OnMove(DirectX::XMFLOAT3 delta) {
	MarkLightAndShadowsDirty();
}

void Light::OnEnable()
{
	MarkLightAndShadowsDirty();
}

void Light::OnDisable()
{
	MarkLightAndShadowsDirty();
}

float Light::GetType()
{
	return type;
}

void Light::SetType(float type)
{
	if (this->type != type) {
		//Point lights can't cast shadows for now
		if (type == 1.0f)
			SetCastsShadows(false);
		this->type = type;
		if(shadowProjector != nullptr)
			shadowProjector->UpdateFieldsByLightType();
		lightArrayDirty = true;
		ComponentManager::Sort<Light>();
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
		this->intensity = intensity;
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
		if (shadowProjector != nullptr && shadowProjector->IsEnabled() && type == 2.0f)
			shadowProjector->SetFarDist(max(range, shadowProjector->GetNearDist() + 0.1f));
		lightArrayDirty = true;
	}
}

bool Light::CastsShadows()
{
	return castsShadows;
}

void Light::SetCastsShadows(bool castsShadows)
{
	//Point lights can't cast shadows for now
	if (type == 1.0f)
		return;
	if (this->castsShadows != castsShadows) {
		this->castsShadows = castsShadows;
		if (castsShadows && shadowProjector == nullptr) {
			shadowProjector = ComponentManager::Instantiate<ShadowProjector>(GetGameEntity());
			shadowProjector->BindLight(shared_from_this());
		}
		shadowProjector->SetEnabled(castsShadows);
		lightArrayDirty = true;
	}
}

std::shared_ptr<ShadowProjector> Light::GetShadowProjector()
{
	return shadowProjector;
}
