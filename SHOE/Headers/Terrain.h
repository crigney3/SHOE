#pragma once

#include "IComponent.h"
#include "Mesh.h"
#include "Material.h"

class Terrain : public IComponent
{
public:
	static void SetDefaults(std::shared_ptr<Mesh> mesh, std::shared_ptr<TerrainMaterial> tMat);
	void Start() override;
	void OnDestroy() override;
	void OnTransform() override;
	void OnParentTransform() override;

	bool DrawBounds;

	void SetMesh(std::shared_ptr<Mesh> newMesh);
	void SetMaterial(std::shared_ptr<TerrainMaterial> newMaterial);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<TerrainMaterial> GetMaterial();

	DirectX::BoundingOrientedBox GetBounds();
private:
	std::string name;

	static std::shared_ptr<Mesh> defaultMesh;
	static std::shared_ptr<TerrainMaterial> defaultTerrainMat;

	std::shared_ptr<Mesh> terrainMesh;
	std::shared_ptr<TerrainMaterial> terrainMaterial;

	DirectX::BoundingOrientedBox bounds;
	void CalculateBounds();
};