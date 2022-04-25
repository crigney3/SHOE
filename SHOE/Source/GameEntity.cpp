#include "../Headers/GameEntity.h"

/**
 * \brief Updates children and attached components with whether the object's parent is enabled
 * \param active Whether this entity's parent is considered enabled
 */
void GameEntity::UpdateHierarchyIsEnabled(bool active, bool head)
{
	if (head || hierarchyIsEnabled != active) {
		if (!head) { 
			hierarchyIsEnabled = active;
			if (enabled && !hierarchyIsEnabled) PropagateEvent(EntityEventType::OnDisable);
		}
		PropagateEvent(EntityEventType::OnEnable);
		for (std::shared_ptr<GameEntity> children : transform->GetChildrenAsGameEntities())
		{
			if (children != nullptr)
				children->UpdateHierarchyIsEnabled(GetEnableDisable());
		}
	}
}

GameEntity::GameEntity(DirectX::XMMATRIX worldIn, std::string name) {
	this->componentList = std::vector<std::shared_ptr<IComponent>>();
	this->componentDeallocList = std::vector<std::function<void(std::shared_ptr<IComponent>)>>();
	this->name = name;
	this->enabled = true;
	this->hierarchyIsEnabled = true;
	transformChangedThisFrame = true;
}

GameEntity::~GameEntity() {
}

/**
 * \brief Delays attaching the transform until the self-reference can be made
 * To be called after instantiation
 */
void GameEntity::Initialize()
{
	this->transform = ComponentManager::Instantiate<Transform>(shared_from_this());
}

void GameEntity::PropagateEvent(EntityEventType event, std::shared_ptr<void> message)
{
	//If the event type requires there to be a message and there isn't one, fail
	if ((EntityEventType::REQUIRES_MESSAGE & 1 << event) && message == nullptr) {
		//Probably should throw but I'll silently fail for now
		return;
	}

	//Propagates the event to all attached components
	if (GetEnableDisable() || event == EntityEventType::OnDisable){
		for (std::shared_ptr<IComponent> component : componentList) {
			if (component->IsEnabled() || (event == EntityEventType::OnDisable && component->IsLocallyEnabled()))
				component->ReceiveEvent(event, message);
		}
	}

	//Handles any event-specific functions within the entity
	switch (event) {
	case EntityEventType::Update:
		if (transformChangedThisFrame) {
			transformChangedThisFrame = false;
			PropagateEvent(EntityEventType::OnTransform);
		}
		break;
	case EntityEventType::OnTransform:
		PropagateEventToChildren(EntityEventType::OnParentTransform, shared_from_this());
		break;
	case EntityEventType::OnMove:
		PropagateEventToChildren(EntityEventType::OnParentMove, shared_from_this());
		transformChangedThisFrame = true;
		break;
	case EntityEventType::OnRotate:
		PropagateEventToChildren(EntityEventType::OnParentRotate, shared_from_this());
		transformChangedThisFrame = true;
		break;
	case EntityEventType::OnScale:
		PropagateEventToChildren(EntityEventType::OnParentScale, shared_from_this());
		transformChangedThisFrame = true;
		break;
	case EntityEventType::OnParentTransform:
		PropagateEventToChildren(EntityEventType::OnParentTransform, message);
		break;
	case EntityEventType::OnParentMove:
		PropagateEventToChildren(EntityEventType::OnParentMove, message);
		break;
	case EntityEventType::OnParentRotate:
		PropagateEventToChildren(EntityEventType::OnParentRotate, message);
		break;
	case EntityEventType::OnParentScale:
		PropagateEventToChildren(EntityEventType::OnParentScale, message);
		break;
	}
}

void GameEntity::PropagateEventToChildren(EntityEventType event, std::shared_ptr<void> message)
{
	for (std::shared_ptr<GameEntity> child : transform->GetChildrenAsGameEntities())
	{
		if (child != nullptr)
			child->PropagateEvent(event, message);
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
	return componentList;
}

/**
 * \brief Frees all of the stored objects in the entity so it can be safely destroyed
 */
void GameEntity::Release()
{
	for(std::shared_ptr<GameEntity> child : transform->GetChildrenAsGameEntities())
	{
		if (child != nullptr) {
			child->GetTransform()->SetParent(nullptr);
		}	
	}
	for (int i = 0; i < componentList.size(); i++) {
		componentList[i]->OnDestroy();
		componentDeallocList[i](componentList[i]);
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
/// <param name="value">New enabled state of the entity</param>
void GameEntity::SetEnableDisable(bool value) {
	if (enabled != value) {
		this->enabled = value;
		if (!value && hierarchyIsEnabled) PropagateEvent(EntityEventType::OnDisable);
		UpdateHierarchyIsEnabled(GetEnableDisable(), true);
	}
}

/// <summary>
/// Whether this GameEntity is enabled and all generations of parent(s) are also enabled
/// </summary>
/// <returns>True if all are enabled</returns>
bool GameEntity::GetEnableDisable() {
	return enabled && hierarchyIsEnabled;
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
/// Removes a component by reference
/// </summary>
/// <param name="component">Component to remove</param>
/// <returns>If the component was successfully removed</returns>
bool GameEntity::RemoveComponent(std::shared_ptr<IComponent> component)
{
	for (int i = 0; i < componentList.size(); i++)
	{
		if (componentList[i] == component)
		{
			componentList[i]->OnDestroy();
			componentDeallocList[i](componentList[i]);
			componentList.erase(componentList.begin() + i);
			componentDeallocList.erase(componentDeallocList.begin() + i);
			return true;
		}
	}
	return false;
}
