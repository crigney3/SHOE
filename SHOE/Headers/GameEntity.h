#pragma once

#include <memory>
#include "GameEntity.fwd.h"
#include "Transform.h"
#include "Light.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
#include "ComponentManager.h"

class GameEntity : public std::enable_shared_from_this<GameEntity>
{
private:
	std::shared_ptr<Transform> transform;
	std::string name;
	bool enabled;
	bool hierarchyIsEnabled;
	bool transformChangedThisFrame;
	bool frozen;

	std::vector<std::shared_ptr<IComponent>> componentList;
	std::vector<std::function<void(std::shared_ptr<IComponent>)>> componentDeallocList;

	void PropagateEvent(EntityEventType event, std::shared_ptr<void> message = nullptr);
	void PropagateEventToChildren(EntityEventType event, std::shared_ptr<void> message = nullptr);

	void UpdateHierarchyIsEnabled(bool active, bool head = false);

	friend class Transform;

public:
	GameEntity(DirectX::XMMATRIX worldIn, std::string name);
	~GameEntity();

	void Initialize();
	
	std::shared_ptr<Transform> GetTransform();

	std::string GetName();
	void SetName(std::string Name);

	void SetEnableDisable(bool value);
	bool GetEnableDisable();
	bool GetHierarchyIsEnabled();

	//Component stuff
	template <typename T>
	std::shared_ptr<T> AddComponent();
	template <> std::shared_ptr<Transform> AddComponent();
	template <> std::shared_ptr<Light> AddComponent();

	template <typename T>
	bool RemoveComponent();
	template <> bool RemoveComponent<Transform>();
	bool RemoveComponent(std::shared_ptr<IComponent> component);

	template <typename T>
	std::shared_ptr<T> GetComponent();
	template <> std::shared_ptr<Transform> GetComponent();

	template <typename T>
	std::vector<std::shared_ptr<T>> GetComponents();
	template <> std::vector<std::shared_ptr<Transform>> GetComponents();

	template <typename T>
	std::shared_ptr<T> GetComponentInChildren();
	template <typename T>
	std::vector<std::shared_ptr<T>> GetComponentsInChildren();

	std::vector<std::shared_ptr<IComponent>> GetAllComponents();
	
	void Release();

	friend class AssetManager;
};

/**
 * \brief Adds a component of the given type to the entity
 * \tparam T Type of component to add
 * \return The newly attached component
 */
template<typename T>
std::shared_ptr<T> GameEntity::AddComponent()
{
	std::shared_ptr<T> component = ComponentManager::Instantiate<T>(shared_from_this());
	componentList.push_back(component);
	componentDeallocList.push_back(ComponentManager::Free<T>);
	return component;
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
 * \brief Special case for lights since they need to be tracked
 * \return A pointer to the new light, or nullptr if MAX_LIGHTS was already reached
 */
template <>
std::shared_ptr<Light> GameEntity::AddComponent<Light>()
{
	if (Light::GetLightArrayCount() == MAX_LIGHTS) {
#if defined(DEBUG) || defined(_DEBUG)
		printf("\nMax lights already exist, cancelling addition of light component.");
#endif
		return nullptr;
	}
	std::shared_ptr<Light> component = ComponentManager::Instantiate<Light>(shared_from_this());
	componentList.push_back(component);
	componentDeallocList.push_back(ComponentManager::Free<Light>);
	return component;
}

/**
 * \brief Frees and removes the first component of the given type
 * \tparam T Type of component to remove
 * \return Whether the component was successfully removed
 */
template<typename T>
bool GameEntity::RemoveComponent()
{
	for(int i = 0; i < componentList.size(); i++)
	{
		if(std::dynamic_pointer_cast<T>(componentList[i]) != nullptr)
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
 * \brief Gets the first component of a given type on this entity
 * \tparam T Type of component to get
 * \return A reference to the component, or nullptr if none are found
 */
template<typename T>
std::shared_ptr<T> GameEntity::GetComponent()
{
	for (std::shared_ptr<IComponent> component : componentList) {
		std::shared_ptr<T> c = std::dynamic_pointer_cast<T>(component);
		if (c != nullptr)
			return c;
	}
	return nullptr;
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
 * \brief Gets all components of a given type on this entity
 * \tparam T Type of components to get
 * \return A list of references to the components
 */
template<typename T>
std::vector<std::shared_ptr<T>> GameEntity::GetComponents()
{
	std::vector<std::shared_ptr<T>> components = std::vector<std::shared_ptr<T>>();
	for (std::shared_ptr<IComponent> c : componentList) {
		std::shared_ptr<T> component = std::dynamic_pointer_cast<T>(c);
		if (component != nullptr)
			components.push_back(component);
	}
	return components;
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
 * \brief Gets the first component of a given type on this entity or its children
 * \tparam T Type of component to get
 * \return A reference to the component, or nullptr if none are found
 */
template<typename T>
std::shared_ptr<T> GameEntity::GetComponentInChildren()
{
	std::shared_ptr<T> component = GetComponent<T>();
	if (component != nullptr)
		return component;

	for(auto& child : transform->GetChildrenEntities())
	{
		component = child->GetComponentInChildren<T>();
		if (component != nullptr)
			return component;
	}

	return nullptr;
}

/**
 * \brief Gets all components of a given type on this entity or its children
 * \tparam T Type of components to get
 * \return A list of references to the components
 */
template<typename T>
std::vector<std::shared_ptr<T>> GameEntity::GetComponentsInChildren()
{
	std::vector<std::shared_ptr<T>> components = GetComponents<T>();

	for (auto& child : transform->GetChildrenEntities())
	{
		components.emplace_back(child->GetComponentsInChildren<T>());
	}

	return components;
}
