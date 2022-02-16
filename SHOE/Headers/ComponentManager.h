#pragma once
#include "ComponentPool.h"

class ComponentManager
{
public:
	template <typename T>
	static std::shared_ptr<T> Instantiate(std::shared_ptr<GameEntity> gameEntity, bool hierarchyIsEnabled);
	template <typename T>
	static void Free(std::shared_ptr<IComponent> component);
	template <typename T>
	static int ActiveCount();
	template <typename T>
	static std::vector<std::shared_ptr<T>> GetAll();
};

template<typename T>
std::shared_ptr<T> ComponentManager::Instantiate(std::shared_ptr<GameEntity> gameEntity, bool hierarchyIsEnabled)
{
	return ComponentPool<T>::Instantiate(gameEntity, hierarchyIsEnabled);
}

template<typename T>
void ComponentManager::Free(std::shared_ptr<IComponent> component)
{
	ComponentPool<T>::Free(component);
}

template<typename T>
int ComponentManager::ActiveCount()
{
	return ComponentPool<T>::GetActiveCount();
}

template<typename T>
std::vector<std::shared_ptr<T>> ComponentManager::GetAll()
{
	return ComponentPool<T>::GetAll();
}
