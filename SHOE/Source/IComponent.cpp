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
 */
void IComponent::Update()
{
}

/**
 * \brief Called once per frame during editing if the GameEntity and component are enabled
 */
void IComponent::EditingUpdate()
{
}

void IComponent::ReceiveEvent(EntityEventType event, std::shared_ptr<void> message)
{
	switch (event) {
	case EntityEventType::Update:
		Update();
		break;
	case EntityEventType::EditingUpdate:
		EditingUpdate();
		break;
	case EntityEventType::OnEnable:
		OnEnable();
		break;
	case EntityEventType::OnDisable:
		OnDisable();
		break;
	case EntityEventType::OnTransform:
		OnTransform();
		break;
	case EntityEventType::OnMove:
		OnMove(*std::static_pointer_cast<DirectX::XMFLOAT3>(message));
		break;
	case EntityEventType::OnRotate:
		OnRotate(*std::static_pointer_cast<DirectX::XMFLOAT3>(message));
		break;
	case EntityEventType::OnScale:
		OnScale(*std::static_pointer_cast<DirectX::XMFLOAT3>(message));
		break;
	case EntityEventType::OnParentTransform:
		OnParentTransform(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnParentMove:
		OnParentMove(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnParentRotate:
		OnParentRotate(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnParentScale:
		OnParentScale(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnCollisionEnter:
		OnCollisionEnter(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnTriggerEnter:
		OnTriggerEnter(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::InCollision:
		InCollision(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::InTrigger:
		InTrigger(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnCollisionExit:
		OnCollisionExit(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnTriggerExit:
		OnTriggerExit(std::static_pointer_cast<GameEntity>(message));
		break;
	case EntityEventType::OnAudioLoad:
		OnAudioLoad(*std::static_pointer_cast<AudioEventPacket>(message));
		break;
	case EntityEventType::OnAudioPlay:
		OnAudioPlay(*std::static_pointer_cast<AudioEventPacket>(message));
		break;
	case EntityEventType::OnAudioPause:
		OnAudioPause(*std::static_pointer_cast<AudioEventPacket>(message));
		break;
	case EntityEventType::OnAudioEnd:
		OnAudioEnd(*std::static_pointer_cast<AudioEventPacket>(message));
		break;
	}
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
 * \brief Called while in a collision with another GameEntity with a collider attached
 * \param other GameEntity that was collided with
 */
void IComponent::InCollision(std::shared_ptr<GameEntity> other)
{
}

/**
 * \brief Called while in another GameEntity's trigger box
 * \param other Entity collided with
 */
void IComponent::InTrigger(std::shared_ptr<GameEntity> other)
{
}

/**
 * \brief Called on exiting a collision with another GameEntity with a collider attached
 * \param other GameEntity that was collided with
 */
void IComponent::OnCollisionExit(std::shared_ptr<GameEntity> other)
{
}

/**
 * \brief Called on exiting another GameEntity's trigger box
 * \param other Entity collided with
 */
void IComponent::OnTriggerExit(std::shared_ptr<GameEntity> other)
{
}

/**
 * \brief Called when this entity's transform is changed
 */
void IComponent::OnTransform()
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
 * \brief Called when a parent's transform is changed
 * \param parent std::shared_ptr<GameEntity> The parent whose transform was changed
 */
void IComponent::OnParentTransform(std::shared_ptr<GameEntity> parent)
{
}

/**
 * \brief Called when a parent is moved
 * \param parent std::shared_ptr<GameEntity> The parent who moved
 */
void IComponent::OnParentMove(std::shared_ptr<GameEntity> parent)
{
}

/**
 * \brief Called when a parent is rotated
 * \param parent std::shared_ptr<GameEntity> The parent who rotated
 */
void IComponent::OnParentRotate(std::shared_ptr<GameEntity> parent)
{
}

/**
 * \brief Called when a parent is scaled
 * \param parent std::shared_ptr<GameEntity> The parent who scaled
 */
void IComponent::OnParentScale(std::shared_ptr<GameEntity> parent)
{
}

/**
 * \brief Called when this entity is changed from disabled to enabled 
 */
void IComponent::OnEnable()
{
}

/**
 * \brief Called when this entity is changed from enabled to disabled
 */
void IComponent::OnDisable()
{
}

/**
 * \brief Called when an audio file is loaded
 * \param parent std::shared_ptr<GameEntity> The parent who scaled
 */
void IComponent::OnAudioLoad(AudioEventPacket audio)
{
}

/**
 * \brief Called when an audio file is played
 * \param audio AudioEventPacket Information about the audio file and broadcast originator
 */
void IComponent::OnAudioPlay(AudioEventPacket audio)
{
}

/**
 * \brief Called when an audio file is paused
 * \param audio AudioEventPacket Information about the audio file and broadcast originator
 */
void IComponent::OnAudioPause(AudioEventPacket audio)
{
}

/**
 * \brief Called when an audio file reaches the end of play
 * \param audio AudioEventPacket Information about the audio file and broadcast originator
 */
void IComponent::OnAudioEnd(AudioEventPacket audio)
{
}

/**
 * \brief 
 * \param gameEntity The GameEntity to be attached to
 * \param hierarchyIsEnabled If that GameEntity is enabled
 */
void IComponent::Bind(std::shared_ptr<GameEntity> gameEntity)
{
	this->gameEntity = gameEntity;
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
	return enabled && gameEntity->GetEnabled();
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
		if (IsEnabled()) OnEnable(); 
		else if(gameEntity->GetEnabled()) OnDisable();
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
