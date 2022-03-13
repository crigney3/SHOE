#include "../Headers/MeshRenderer.h"

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
	this->mat = newMaterial;
}