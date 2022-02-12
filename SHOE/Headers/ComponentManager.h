#pragma once
#include <map>
#include "ComponentPool.h"
#include <string>
#include <vector>
#include <functional>

class ComponentManager
{
public:
	static void Initialize();
	template <typename T>
	static T* Instantiate(GameEntity* gameEntity);
	template <typename T>
	static void Free(T* component);
	template <typename T>
	static int ActiveCount();

	static void DumpAll();
private:
	static std::vector<std::function<void()>> poolDeallocateCalls;
};

template<typename T>
inline T* ComponentManager::Instantiate(GameEntity* gameEntity)
{
	bool newPool = false;
	T* component = ComponentPool<T>()->Instantiate(gameEntity, &newPool);

	//Makes sure memory will be handled properly
	if (newPool) {
		poolDeallocateCalls.push_back(ComponentPool<T>()->DumpAll);
	}

	return component;
}

template<typename T>
inline void ComponentManager::Free(T* component)
{
	ComponentPool<T>()->Free(component);
}

template<typename T>
inline int ComponentManager::ActiveCount()
{
	return ComponentPool<T>()::GetActiveCount();
}
