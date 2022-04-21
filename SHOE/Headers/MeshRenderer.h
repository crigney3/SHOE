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
	void OnDestroy() override;
	void OnRotate(DirectX::XMFLOAT3 delta) override;
	void OnScale(DirectX::XMFLOAT3 delta) override;

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<Material> GetMaterial();

	void SetMesh(std::shared_ptr<Mesh> newMesh);
	void SetMaterial(std::shared_ptr<Material> newMaterial);

	DirectX::BoundingOrientedBox GetBounds();
private:
	static std::shared_ptr<Mesh> defaultMesh;
	static std::shared_ptr<Material> defaultMat;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> mat;

	DirectX::BoundingOrientedBox bounds;
	void CalculateBounds();
};