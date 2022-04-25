#pragma once
#include "ComponentPool.h"

class ComponentManager
{
public:
	template <typename T>
	static std::shared_ptr<T> Instantiate(std::shared_ptr<GameEntity> gameEntity);
	template <typename T>
	static void Free(std::shared_ptr<IComponent> component);
	template <typename T>
	static int ActiveCount();
	template <typename T>
	static std::vector<std::shared_ptr<T>> GetAll();
	template <typename T>
	static std::vector<std::shared_ptr<T>> GetAllEnabled();
};

/**
 * \brief Grabs an open component from the relevant pool
 * \tparam T The type of component to grab
 * \param gameEntity The entity the component is to be attached to
 * \param hierarchyIsEnabled Whether that entity is enabled in the hierarchy
 * \return A new component from the pool
 */
template<typename T>
std::shared_ptr<T> ComponentManager::Instantiate(std::shared_ptr<GameEntity> gameEntity)
{
	return ComponentPool<T>::Instantiate(gameEntity);
}

/**
 * \brief Frees a component to be reused later
 * \tparam T Type of component to free
 * \param component The component to be returned to the free pool
 */
template<typename T>
void ComponentManager::Free(std::shared_ptr<IComponent> component)
{
	ComponentPool<T>::Free(component);
}

/**
 * \brief Gets how many components are in use in a given pool
 * \tparam T The pool type to poll
 * \return How many components are currently in use
 */
template<typename T>
int ComponentManager::ActiveCount()
{
	return ComponentPool<T>::GetActiveCount();
}

/**
 * \brief Gets a list of all in use components from a pool
 * \tparam T Type of pool to poll
 * \return A list of in use components
 */
template<typename T>
std::vector<std::shared_ptr<T>> ComponentManager::GetAll()
{
	return ComponentPool<T>::GetAll();
}

/**
 * \brief Gets a list of all enabled components from a pool
 * \tparam T Type of pool to poll
 * \return A list of in use and enabled components
 */
template<typename T>
std::vector<std::shared_ptr<T>> ComponentManager::GetAllEnabled()
{
	return ComponentPool<T>::GetAllEnabled();
}
