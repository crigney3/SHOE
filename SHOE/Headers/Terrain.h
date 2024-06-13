#pragma once

#include "IComponent.h"
#include "Mesh.h"
#include "Material.h"

struct HeightMap {
	unsigned int numVertices;
	unsigned int numIndices;
	std::vector<unsigned short> heights;
	std::vector<float> finalHeights;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<XMFLOAT3> triangleNormals;
	std::string name;
	std::string filenameKey;
};

class Terrain : public IComponent
{
public:
	static void SetDefaults(std::shared_ptr<Mesh> mesh, std::shared_ptr<TerrainMaterial> tMat);
	void OnDestroy() override;

	bool DrawBounds;

	void SetMesh(std::shared_ptr<Mesh> newMesh);
	void SetMaterial(std::shared_ptr<TerrainMaterial> newMaterial);
	void SetHeightMap(std::shared_ptr<HeightMap> newHeightMap);
	void SetHeightMapName(std::string newName);

	std::shared_ptr<Mesh> GetMesh();
	std::shared_ptr<TerrainMaterial> GetMaterial();
	std::shared_ptr<HeightMap> GetHeightMap();
	std::string GetHeightMapName();
	std::string GetHeightMapFileNameKey();

	DirectX::BoundingOrientedBox GetBounds();
private:
	std::string name;

	static std::shared_ptr<Mesh> defaultMesh;
	static std::shared_ptr<TerrainMaterial> defaultTerrainMat;

	std::shared_ptr<Mesh> terrainMesh;
	std::shared_ptr<TerrainMaterial> terrainMaterial;

	std::shared_ptr<HeightMap> terrainHeight;

	DirectX::BoundingOrientedBox bounds;
	void CalculateBounds();
	void Start() override;
	void OnTransform() override;
	void OnParentTransform(std::shared_ptr<GameEntity> parent) override;
};