#pragma once
#include "IComponent.h"

#define POOL_SIZE 30;

template <typename T>
class ComponentPool
{
public:
	static T* Instantiate(GameEntity* gameEntity, bool* generatedNewPool);
	static void Free(T* component);
	static int GetActiveCount();
	static void DumpAll();

private:
	static int activeCount = 0;
	static IComponent* pool = nullptr;
	static T* unallocatedHead;
};

template<typename T>
inline T* ComponentPool<T>::Instantiate(GameEntity* gameEntity, bool* generatedNewPool)
{
	//Allocates a new pool if there is no active one
	if (pool == nullptr) {
		pool = new T[POOL_SIZE];
		unallocatedHead = pool[0];
		for (int i = 0; i < POOL_SIZE - 1; i++)
			pool[i].Free(pool[i + 1]);
		*generatedNewPool = true;
	}

	//If out of space
	if (unallocatedHead == nullptr) {
		throw;
	}

	T* component = unallocatedHead;
	unallocatedHead = component->GetNext();
	component->Bind(gameEntity);
	activeCount++;
	return component;
}

template<typename T>
inline void ComponentPool<T>::Free(T* component)
{
	component->Free(unallocatedHead);
	unallocatedHead = component;
	activeCount--;
}

template<typename T>
inline int ComponentPool<T>::GetActiveCount()
{
	return activeCount;
}

template<typename T>
inline void ComponentPool<T>::DumpAll()
{
	delete[] pool;
}
