#include "../Headers/MeshRenderer.h"
#include "..\Headers\AssetManager.h"

std::shared_ptr<Mesh> MeshRenderer::defaultMesh = nullptr;
std::shared_ptr<Material> MeshRenderer::defaultMat = nullptr;

/// <summary>
/// Sets the default mesh and material for a newly allocated MeshRenderer
/// </summary>
/// <param name="mesh">Default Mesh</param>
/// <param name="mat">Default Material</param>
void MeshRenderer::SetDefaults(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat)
{
	defaultMesh = mesh;
	defaultMat = mat;
}

/// <summary>
/// Sets the members to the defaults for a newly allocated MeshRenderer
/// </summary>
void MeshRenderer::Start()
{
	mesh = defaultMesh;
	mat = defaultMat;
}

/// <summary>
/// Get the mesh this MeshRenderer renders
/// </summary>
/// <returns>A pointer to the mesh</returns>
std::shared_ptr<Mesh> MeshRenderer::GetMesh() {
	return this->mesh;
}

/// <summary>
/// Get the material this MeshRenderer renders
/// </summary>
/// <returns>A pointer to the material</returns>
std::shared_ptr<Material> MeshRenderer::GetMaterial() {
	return this->mat;
}

/// <summary>
/// Set the mesh this MeshRenderer renders
/// </summary>
/// <param name="newMesh">New mesh to render</param>
void MeshRenderer::SetMesh(std::shared_ptr<Mesh> newMesh) {
	this->mesh = newMesh;
}

/// <summary>
/// Set the material this MeshRenderer renders
/// </summary>
/// <param name="newMesh">New material to render</param>
void MeshRenderer::SetMaterial(std::shared_ptr<Material> newMaterial) {
	AssetManager::materialSortDirty = true;
	this->mat = newMaterial;
}