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

void IComponent::Bind(GameEntity* gameEntity)
{
	this->gameEntity = gameEntity;
	Start();
}

void IComponent::Free(IComponent* next)
{
	this->next = next;
}

bool IComponent::IsEnabled()
{
	return enabled;
}

void IComponent::SetEnabled(bool enabled)
{
	this->enabled = enabled;
}

IComponent* IComponent::GetNext()
{
	return next;
}

GameEntity* IComponent::GetGameEntity()
{
	return gameEntity;
}
