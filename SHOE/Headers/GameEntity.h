#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>
#include "Camera.h"
#include "Lights.h"
#include "GameEntity.fwd.h"
#include "ComponentManager.h"
#include "ComponentPacket.h"

class GameEntity
{
private:
	Transform* transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> mat;
	std::string name;
	bool enabled;

	void SetMaterial(std::shared_ptr<Material> newMaterial);
	void SetMesh(std::shared_ptr<Mesh> newMesh);

	std::vector<ComponentPacket> componentList;

public:
	GameEntity(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX worldIn, std::shared_ptr<Material> mat, std::string name = "GameObject");
	~GameEntity();

	void Update();

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();
	std::shared_ptr<Material> GetMaterial();

	std::string GetName();
	void SetName(std::string Name);

	void SetEnableDisable(bool value);
	bool GetEnableDisable();

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2);
	void DrawFromVerts(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2);

	//Component stuff
	template <typename T>
	T* AddComponent();
	template <typename T>
	bool RemoveComponent();
	template <typename T>
	T* GetComponent();
	template <typename T>
	std::vector<T> GetComponents();
	template <typename T>
	T* GetComponentInChildren();
	template <typename T>
	std::vector<T> GetComponentsInChildren();

	friend class AssetManager;
};

template<typename T>
inline T* GameEntity::AddComponent()
{
	T* component = ComponentManager::Instantiate<T>(this);
	componentList.push_back(ComponentPacket(component, ComponentManager::Free<T>));
	return component;
}

template<typename T>
inline bool GameEntity::RemoveComponent()
{
	for(int i = 0; i < componentList.size(); i++)
	{
		if(dynamic_cast<T*>(componentList[i].component) != nullptr)
		{
			componentList[i].deallocator(componentList[i].component);
			componentList.erase(componentList.begin() + i);
			return true;
		}
	}
	return false;
}
