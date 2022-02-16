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
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> mat;
	std::string name;
	bool enabled;
	bool hierarchyIsEnabled;

	void SetMaterial(std::shared_ptr<Material> newMaterial);
	void SetMesh(std::shared_ptr<Mesh> newMesh);

	std::vector<std::shared_ptr<ComponentPacket>> componentList;

	void UpdateHierarchyIsEnabled(bool active);

public:
	GameEntity(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX worldIn, std::shared_ptr<Material> mat, std::string name);
	~GameEntity();

	void Initialize();
	void Update();
	void OnCollisionEnter(std::shared_ptr<GameEntity> other);
	void OnTriggerEnter(std::shared_ptr<GameEntity> other);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Transform> GetTransform();
	std::shared_ptr<Material> GetMaterial();

	std::string GetName();
	void SetName(std::string Name);

	void SetEnableDisable(bool value);
	bool GetEnableDisable();
	bool GetHierarchyIsEnabled();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2);
	void DrawFromVerts(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2);

	//Component stuff
	template <typename T>
	std::shared_ptr<T> AddComponent();
	template <typename T>
	bool RemoveComponent();
	template <typename T>
	std::shared_ptr<T> GetComponent();
	template <typename T>
	std::vector<std::shared_ptr<T>> GetComponents();
	template <typename T>
	std::shared_ptr<T> GetComponentInChildren();
	template <typename T>
	std::vector<std::shared_ptr<T>> GetComponentsInChildren();

	void Release();

	friend class AssetManager;
};

template<typename T>
std::shared_ptr<T> GameEntity::AddComponent()
{
	std::shared_ptr<T> component = ComponentManager::Instantiate<T>(shared_from_this(), this->GetHierarchyIsEnabled());
	componentList.push_back(std::make_shared<ComponentPacket>(component, ComponentManager::Free<T>));
	return component;
}

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
			return true;
		}
	}
	return false;
}

template<typename T>
std::shared_ptr<T> GameEntity::GetComponent()
{
	for (std::shared_ptr<ComponentPacket> packet : componentList) {
		if (std::dynamic_pointer_cast<T>(packet->component).get() != nullptr)
			return packet->component;
	}
	return nullptr;
}

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
