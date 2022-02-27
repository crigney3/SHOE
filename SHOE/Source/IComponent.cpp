#include "..\Headers\IComponent.h"

/**
 * \brief Called when the component is added to a GameEntity
 */
void IComponent::Start()
{
}

/**
 * \brief Called once per frame if the GameEntity and component are enabled
 * \param deltaTime Time since last frame
 * \param totalTime Time since program start
 */
void IComponent::Update(float deltaTime, float totalTime)
{
}

/**
 * \brief Called when component is detached from a GameEntity
 */
void IComponent::OnDestroy()
{
}

/**
 * \brief Called on entering a collision with another GameEntity with a collider attached
 * \param other GameEntity that was collided with
 */
void IComponent::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
}

/**
 * \brief Called on entering another GameEntity's trigger box
 * \param other Entity collided with
 */
void IComponent::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
}

/**
 * \brief 
 * \param gameEntity The GameEntity to be attached to
 * \param hierarchyIsEnabled If that GameEntity is enabled
 */
void IComponent::Bind(std::shared_ptr<GameEntity> gameEntity, bool hierarchyIsEnabled)
{
	this->gameEntity = gameEntity;
	this->hierarchyIsEnabled = hierarchyIsEnabled;
	Start();
}

/**
 * \brief Frees a component when it is to be detached from a GameEntity
 */
void IComponent::Free()
{
	gameEntity = nullptr;
}

/**
 * \brief Returns whether this component and the GameEntity it is on is enabled
 * \return 
 */
bool IComponent::IsEnabled()
{
	return enabled && hierarchyIsEnabled;
}

void IComponent::SetEnabled(bool enabled)
{
	this->enabled = enabled;
}

/**
 * \return The GameEntity this is bound to
 */
std::shared_ptr<GameEntity> IComponent::GetGameEntity()
{
	return gameEntity;
}

void IComponent::UpdateHierarchyIsEnabled(bool active)
{
	hierarchyIsEnabled = active;
}
