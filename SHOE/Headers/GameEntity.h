#pragma once

#include <memory>
#include "GameEntity.fwd.h"
#include "Transform.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
#include "ComponentManager.h"
#include "ComponentPacket.h"

class GameEntity : public std::enable_shared_from_this<GameEntity>
{
private:
	std::shared_ptr<Transform> transform;
	std::string name;
	bool enabled;
	bool hierarchyIsEnabled;

	std::vector<std::shared_ptr<IComponent>> rawComponentList;
	std::vector<std::shared_ptr<ComponentPacket>> componentList;

	void UpdateHierarchyIsEnabled(bool active);

	friend class Transform;

public:
	GameEntity(DirectX::XMMATRIX worldIn, std::string name);
	~GameEntity();

	void Initialize();
	void Update(float deltaTime, float totalTime);
	void OnCollisionEnter(std::shared_ptr<GameEntity> other);
	void OnTriggerEnter(std::shared_ptr<GameEntity> other);
	
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

	template <typename T>
	bool RemoveComponent();
	template <> bool RemoveComponent<Transform>();

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
	std::shared_ptr<T> component = ComponentManager::Instantiate<T>(shared_from_this(), this->GetHierarchyIsEnabled());
	componentList.push_back(std::make_shared<ComponentPacket>(component, ComponentManager::Free<T>));
	rawComponentList.push_back(component);
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
		if(std::dynamic_pointer_cast<T>(componentList[i]->component).get() != nullptr)
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

/**
 * \brief Gets the first component of a given type on this entity
 * \tparam T Type of component to get
 * \return A reference to the component, or nullptr if none are found
 */
template<typename T>
std::shared_ptr<T> GameEntity::GetComponent()
{
	for (std::shared_ptr<ComponentPacket> packet : componentList) {
		if (std::dynamic_pointer_cast<T>(packet->component).get() != nullptr)
			return packet->component;
	}
	return nullptr;
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
	for (std::shared_ptr<ComponentPacket> packet : componentList) {
		std::shared_ptr<T> component = std::dynamic_pointer_cast<T>(packet->component);
		if (component.get() != nullptr)
			components.push_back(component);
	}
	return components;
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

	for(auto& child : transform->GetChildrenAsGameEntities())
	{
		component = child->GetComponentInChildren<T>();
		if (component.get() != nullptr)
			return component;
	}

	return std::shared_ptr<T>();
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

	for (auto& child : transform->GetChildrenAsGameEntities())
	{
		components.emplace_back(child->GetComponentsInChildren<T>());
	}

	return components;
}
