#pragma once
#include "IComponent.h"
#include "Mesh.h"
#include "Material.h"
class MeshRenderer : public IComponent
{
public:
	static void SetDefaults(
		std::shared_ptr<Mesh> mesh,
		std::shared_ptr<Material> mat
	);
	void Start() override;
	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();

	void SetMesh(std::shared_ptr<Mesh> newMesh);
	void SetMaterial(std::shared_ptr<Material> newMaterial);
private:
	static std::shared_ptr<Mesh> defaultMesh;
	static std::shared_ptr<Material> defaultMat;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> mat;
};