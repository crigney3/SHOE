#include "..\Headers\IComponent.h"

void IComponent::Start()
{
}

void IComponent::Update()
{
}

void IComponent::OnDestroy()
{
}

void IComponent::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
}

void IComponent::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
}

void IComponent::Bind(std::shared_ptr<GameEntity> gameEntity, bool hierarchyIsEnabled)
{
	this->gameEntity = gameEntity;
	this->hierarchyIsEnabled = hierarchyIsEnabled;
	Start();
}

void IComponent::Free()
{
	gameEntity = nullptr;
}

bool IComponent::IsEnabled()
{
	return enabled && hierarchyIsEnabled;
}

void IComponent::SetEnabled(bool enabled)
{
	this->enabled = enabled;
}

std::shared_ptr<GameEntity> IComponent::GetGameEntity()
{
	return gameEntity;
}

void IComponent::UpdateHierarchyIsEnabled(bool active)
{
	hierarchyIsEnabled = active;
}
