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