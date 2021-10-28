#pragma once

#include "Transform.h"
#include "Mesh.h"
#include "Material.h"
#include <memory>
#include "Camera.h"
#include "Lights.h"
#include "GameEntity.fwd.h"

class GameEntity
{
private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> mat;
	std::string name;
public:
	GameEntity(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX worldIn, std::shared_ptr<Material> mat, std::string name = "GameObject");
	~GameEntity();

	std::shared_ptr<Mesh> GetMesh();
	Transform* GetTransform();
	std::shared_ptr<Material> GetMaterial();

	std::string GetName();
	void SetName(std::string Name);

	void Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2);
	void DrawFromVerts(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2);
	//void GenShadows(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> shadowCam);
};

