#pragma once
#include "GameEntity.fwd.h"
#include <vector>

class IComponent
{
public:
	virtual void Start();
	virtual void Update();
	virtual void OnDestroy();
	void Bind(GameEntity* gameEntity);
	void Free(IComponent* next);

	template <typename T>
	T* GetComponent();
	template <typename T>
	T* GetComponents();
	template <typename T>
	std::vector<T> GetComponentInChildren();
	template <typename T>
	std::vector<T> GetComponentsInChildren();

	bool IsEnabled();
	void SetEnabled(bool enabled);
	IComponent* GetNext();
	GameEntity* GetGameEntity();
private:
	union
	{
		IComponent* next;
		GameEntity* gameEntity;
	};

	bool enabled = true;
};

template<typename T>
inline T* IComponent::GetComponent()
{
	return nullptr;
}

template<typename T>
inline T* IComponent::GetComponents()
{
	return nullptr;
}

template<typename T>
inline std::vector<T> IComponent::GetComponentInChildren()
{
	return std::vector<T>();
}

template<typename T>
inline std::vector<T> IComponent::GetComponentsInChildren()
{
	return std::vector<T>();
}
