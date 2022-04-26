#include "..\Headers\IComponent.h"
#include "..\Headers\GameEntity.h"
#include "..\Headers\Light.h"

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
 * \brief Called when this entity's transform is changed
 */
void IComponent::OnTransform()
{
}

/**
 * \brief Called when this parent's transform is changed
 */
void IComponent::OnParentTransform()
{
}

/**
 * \brief Called when this entity's position is changed
 * \param delta DirectX::XMFLOAT3 The delta of this move
 */
void IComponent::OnMove(DirectX::XMFLOAT3 delta)
{
}

/**
 * \brief Called when this entity's rotation is changed
 * \param delta DirectX::XMFLOAT3 The delta of this rotation
 */
void IComponent::OnRotate(DirectX::XMFLOAT3 delta)
{
}

/**
 * \brief Called when this entity's scale is changed
 * \param delta DirectX::XMFLOAT3 The delta of this scale
 */
void IComponent::OnScale(DirectX::XMFLOAT3 delta)
{
}

/**
 * \brief Called when this entity's enabled state is changed
 * \param newState bool The new state of the component
 */
void IComponent::OnEnabledChanged(bool newState)
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
 */
bool IComponent::IsEnabled()
{
	return enabled && hierarchyIsEnabled;
}

/// <summary>
/// Whether this component is enabled, irregardless of if the GameEntity it's on is
/// </summary>
/// <returns>True if enabled</returns>
bool IComponent::IsLocallyEnabled()
{
	return enabled;
}

/// <summary>
/// Sets if this component is locally enabled
/// </summary>
void IComponent::SetEnabled(bool enabled)
{
	if (this->enabled != enabled) {
		this->enabled = enabled;
		OnEnabledChanged(enabled);
	}
}

/**
 * \return The GameEntity this is bound to
 */
std::shared_ptr<GameEntity> IComponent::GetGameEntity()
{
	return gameEntity;
}

/// <summary>
/// Shortcut to get the Transform of the GameEntity this is attached to
/// </summary>
/// <returns>A pointer to the Transform</returns>
std::shared_ptr<Transform> IComponent::GetTransform()
{
	return gameEntity->GetTransform();
}

/// <summary>
/// Called when the GameEntity this is attached to changes enabled state
/// </summary>
/// <param name="active">Is the GameEntity globally enabled</param>
void IComponent::UpdateHierarchyIsEnabled(bool active)
{
	hierarchyIsEnabled = active;
}
