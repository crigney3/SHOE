#include "../Headers/MeshRenderer.h"
#include "..\Headers\AssetManager.h"

std::shared_ptr<Mesh> MeshRenderer::defaultMesh = nullptr;
std::shared_ptr<Material> MeshRenderer::defaultMat = nullptr;

void MeshRenderer::SetDefaults(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> mat)
{
	defaultMesh = mesh;
	defaultMat = mat;
}

void MeshRenderer::Start()
{
	mesh = defaultMesh;
	mat = defaultMat;
}

std::shared_ptr<Mesh> MeshRenderer::GetMesh() {
	return this->mesh;
}

std::shared_ptr<Material> MeshRenderer::GetMaterial() {
	return this->mat;
}

void MeshRenderer::SetMesh(std::shared_ptr<Mesh> newMesh) {
	this->mesh = newMesh;
}

void MeshRenderer::SetMaterial(std::shared_ptr<Material> newMaterial) {
	AssetManager::materialSortDirty = true;
	this->mat = newMaterial;
}