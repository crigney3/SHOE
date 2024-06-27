#include "..\Headers\Terrain.h"
#include "..\Headers\Transform.h"

std::shared_ptr<Mesh> Terrain::defaultMesh = nullptr;
std::shared_ptr<TerrainMaterial> Terrain::defaultTerrainMat = nullptr;

/// <summary>
/// Sets the default mesh and material for a newly allocated Terrain
/// </summary>
/// <param name="mesh">Default Mesh</param>
/// <param name="mat">Default Material</param>
void Terrain::SetDefaults(std::shared_ptr<Mesh> mesh, std::shared_ptr<TerrainMaterial> mat)
{
	defaultMesh = mesh;
	defaultTerrainMat = mat;
}

/// <summary>
/// Sets the members to the defaults for a newly allocated Terrain
/// </summary>
void Terrain::Start()
{
	SetMesh(defaultMesh);
	SetMaterial(defaultTerrainMat);
	DrawBounds = false;
}

/// <summary>
/// Removes references to the internal mesh and material on destruction
/// </summary>
void Terrain::OnDestroy()
{
	terrainMesh = nullptr;
	terrainMaterial = nullptr;
}

void Terrain::OnTransform()
{
	CalculateBounds();
}

void Terrain::OnParentTransform(std::shared_ptr<GameEntity> parent)
{
	CalculateBounds();
}

/// <summary>
/// Sets this terrain's vertices/normals/height etc to a loaded heightmap.
/// </summary>
/// <param name="newHeightMap">See AssetManager's functions for loading heightmaps.</param>
void Terrain::SetHeightMap(std::shared_ptr<HeightMap> newHeightMap) {
	this->terrainHeight = newHeightMap;
}

/// <summary>
/// Sets the name of the heightmap. Mainly useful for the UI.
/// </summary>
/// <param name="newName"></param>
void Terrain::SetHeightMapName(std::string newName) {
	this->terrainHeight->name = newName;
}

/// <summary>
/// Returns all the data of this terrain's heightmap.
/// </summary>
/// <returns></returns>
std::shared_ptr<HeightMap> Terrain::GetHeightMap() {
	return this->terrainHeight;
}

/// <summary>
/// Returns this terrain's heightmap's name. Makes UI code easier to read.
/// </summary>
/// <returns></returns>
std::string Terrain::GetHeightMapName() {
	return this->terrainHeight->name;
}

/// <summary>
/// Returns this terrain's heightmap's filename key. Makes saving/loading code easier to read.
/// </summary>
/// <returns></returns>
std::string Terrain::GetHeightMapFileNameKey() {
	return this->terrainHeight->filenameKey;
}

/// <summary>
/// Get the terrainMesh this Terrain renders
/// </summary>
/// <returns>A pointer to the mesh</returns>
std::shared_ptr<Mesh> Terrain::GetMesh() {
	return this->terrainMesh;
}

/// <summary>
/// Get the material this Terrain renders
/// </summary>
/// <returns>A pointer to the material</returns>
std::shared_ptr<TerrainMaterial> Terrain::GetMaterial() {
	return this->terrainMaterial;
}

/// <summary>
/// Set the mesh this Terrain renders
/// </summary>
/// <param name="newMesh">New mesh to render</param>
void Terrain::SetMesh(std::shared_ptr<Mesh> newMesh) {
	this->terrainMesh = newMesh;
	CalculateBounds();
}

/// <summary>
/// Set the material this Terrain renders
/// </summary>
/// <param name="newMesh">New material to render</param>
void Terrain::SetMaterial(std::shared_ptr<TerrainMaterial> newMaterial) {
	this->terrainMaterial = newMaterial;
}

DirectX::BoundingOrientedBox Terrain::GetBounds()
{
	return bounds;
}

void Terrain::CalculateBounds()
{
	bounds = DirectX::BoundingOrientedBox(GetTransform()->GetGlobalPosition(), DirectX::XMFLOAT3(256.0f, 2.0f, 256.0f), GetTransform()->GetGlobalRotation());
	//bounds.Transform(bounds, DirectX::XMLoadFloat4x4(&GetTransform()->GetWorldMatrix()));
}