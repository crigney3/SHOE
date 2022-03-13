#pragma once
#include "IComponent.h"
#include "Mesh.h"
#include "Material.h"
#include "Camera.h"
class MeshRenderer : public IComponent
{
private:
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> mat;
public:
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();

	void SetMesh(std::shared_ptr<Mesh> newMesh);
	void SetMaterial(std::shared_ptr<Material> newMaterial);
};

