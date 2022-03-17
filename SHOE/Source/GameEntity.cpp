#include "../Headers/GameEntity.h"

/**
 * \brief Updates children and attached components with whether the object's parent is enabled
 * \param active Whether this entity's parent is considered enabled
 */
void GameEntity::UpdateHierarchyIsEnabled(bool active)
{
	for (std::shared_ptr<ComponentPacket> packet : componentList)
	{
		packet->component->UpdateHierarchyIsEnabled(GetEnableDisable());
	}
	for (std::shared_ptr<GameEntity> children : transform->GetChildrenAsGameEntities())
	{
		children->UpdateHierarchyIsEnabled(GetEnableDisable());
	}
}

GameEntity::GameEntity(DirectX::XMMATRIX worldIn, std::string name) {
	this->componentList = std::vector<std::shared_ptr<ComponentPacket>>();
	this->name = name;
	this->enabled = true;
	this->hierarchyIsEnabled = true;
}

GameEntity::~GameEntity() {
}

/**
 * \brief Delays attaching the transform until the self-reference can be made
 * To be called after instantiation
 */
void GameEntity::Initialize()
{
	this->transform = ComponentManager::Instantiate<Transform>(shared_from_this(), this->GetHierarchyIsEnabled());
}

/**
 * \brief Updates all attached components
 */
void GameEntity::Update(float deltaTime, float totalTime)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->Update(deltaTime, totalTime);
		}
	}
}

/**
 * \brief Called on entering a collision with another GameEntity with a collider attached
 * \param other Entity collided with
 */
void GameEntity::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->OnCollisionEnter(other);
		}
	}
}

/**
 * \brief Called on entering another GameEntity's trigger box
 * \param other Entity collided with
 */
void GameEntity::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->OnTriggerEnter(other);
		}
	}
}

std::shared_ptr<Transform> GameEntity::GetTransform() {
	return transform;
}

/**
 * \brief Special case for transform, cannot have multiple transforms
 * \return This entity's transform
 */
template <>
std::shared_ptr<Transform> GameEntity::AddComponent<Transform>()
{
	//Does nothing, cannot have multiple transforms
	return transform;
}

/**
 * \brief Special case for transform, does nothing since the transform has the same lifetime as the entity
 * \return False
 */
template <>
bool GameEntity::RemoveComponent<Transform>()
{
	return false;
}

/**
 * \brief Special case for transform
 * \return This entity's transform
 */
template <>
std::shared_ptr<Transform> GameEntity::GetComponent<Transform>()
{
	return transform;
}

/**
 * \brief Special case for transform
 * \return This entity's transform
 */
template <>
std::vector<std::shared_ptr<Transform>> GameEntity::GetComponents<Transform>()
{
	return std::vector<std::shared_ptr<Transform>> { transform };
}

/**
 * \brief Frees all of the stored objects in the entity so it can be safely destroyed
 */
void GameEntity::Release()
{
	/*for(std::shared_ptr<GameEntity> child : transform->GetChildrenAsGameEntities())
	{
		child->GetTransform()->SetParent(nullptr);
	}*/
	for (std::shared_ptr<ComponentPacket> packet : componentList) {
		packet->component->OnDestroy();
		packet->deallocator(packet->component);
	}
	ComponentManager::Free<Transform>(transform);
}

std::string GameEntity::GetName() {
	return this->name;
}

void GameEntity::SetName(std::string Name) {
	this->name = Name;
}

void GameEntity::SetEnableDisable(bool value) {
	this->enabled = value;
	UpdateHierarchyIsEnabled(hierarchyIsEnabled);
}

bool GameEntity::GetEnableDisable() {
	return this->enabled && this->hierarchyIsEnabled;
}

bool GameEntity::GetHierarchyIsEnabled()
{
	return hierarchyIsEnabled;
}
