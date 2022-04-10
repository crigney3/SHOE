#include "../Headers/GameEntity.h"

/**
 * \brief Updates children and attached components with whether the object's parent is enabled
 * \param active Whether this entity's parent is considered enabled
 */
void GameEntity::UpdateHierarchyIsEnabled(bool active, bool head)
{
	if (head || hierarchyIsEnabled != active) {
		if(!head) hierarchyIsEnabled = active;
		if (HasLightAttached()) Light::MarkDirty();
		for (std::shared_ptr<ComponentPacket> packet : componentList)
		{
			packet->component->UpdateHierarchyIsEnabled(GetEnableDisable());
		}
		for (std::shared_ptr<GameEntity> children : transform->GetChildrenAsGameEntities())
		{
			if (children.get() != NULL)
				children->UpdateHierarchyIsEnabled(GetEnableDisable());
		}
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

/// <summary>
/// Same as GetComponent<Transform>()
/// </summary>
std::shared_ptr<Transform> GameEntity::GetTransform() {
	return transform;
}

/**
 * \brief Returns a list of all attached components
 */
std::vector<std::shared_ptr<IComponent>> GameEntity::GetAllComponents()
{
	return rawComponentList;
}

/**
 * \brief Returns how many lights are attached to the entity.
 */
int GameEntity::GetAttachedLightCount() {
	return attachedLightCount;
}

/**
 * \brief Frees all of the stored objects in the entity so it can be safely destroyed
 */
void GameEntity::Release()
{
	for(std::shared_ptr<GameEntity> child : transform->GetChildrenAsGameEntities())
	{
		if (child != NULL) {
			child->GetTransform()->SetParent(nullptr);
		}	
	}
	for (std::shared_ptr<ComponentPacket> packet : componentList) {
		packet->component->OnDestroy();
		packet->deallocator(packet->component);
	}
	transform->OnDestroy();
	ComponentManager::Free<Transform>(transform);
}

std::string GameEntity::GetName() {
	return this->name;
}

void GameEntity::SetName(std::string Name) {
	this->name = Name;
}

/// <summary>
/// Sets whether this GameEntity is enabled
/// </summary>
/// <param name="value"></param>
void GameEntity::SetEnableDisable(bool value) {
	if (enabled != value) {
		this->enabled = value;
		UpdateHierarchyIsEnabled(GetEnableDisable(), true);
	}
}

/// <summary>
/// Whether this GameEntity is enabled and all generations of parent(s) are also enabled
/// </summary>
/// <returns>True if all are enabled</returns>
bool GameEntity::GetEnableDisable() {
	return this->enabled && this->hierarchyIsEnabled;
}

/// <summary>
/// Are all generations of parent(s) of this Entity enabled
/// </summary>
/// <returns>True if all are enabled</returns>
bool GameEntity::GetHierarchyIsEnabled()
{
	return hierarchyIsEnabled;
}

/// <summary>
/// Returns whether this has at least one light component attached
/// </summary>
/// <returns>If this has a light component</returns>
bool GameEntity::HasLightAttached()
{
	return attachedLightCount > 0;
}

/// <summary>
/// Removes a component by reference
/// </summary>
/// <param name="component">Component to remove</param>
/// <returns>If the component was successfully removed</returns>
bool GameEntity::RemoveComponent(std::shared_ptr<IComponent> component)
{
	for (int i = 0; i < componentList.size(); i++)
	{
		if (componentList[i]->component == component)
		{
			componentList[i]->component->OnDestroy();
			componentList[i]->deallocator(componentList[i]->component);
			componentList.erase(componentList.begin() + i);
			rawComponentList.erase(rawComponentList.begin() + i);
			return true;
		}
	}
	return false;
}
