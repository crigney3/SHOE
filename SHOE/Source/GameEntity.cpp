#include "../Headers/GameEntity.h"

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX worldIn, std::shared_ptr<Material> mat, std::string name) {
	this->mesh = mesh;
	this->transform = Transform(worldIn);
	this->transform.SetGameEntity(this);
	this->mat = mat;
	this->name = name;
	this->enabled = true;
}

GameEntity::~GameEntity() {

}

std::shared_ptr<Mesh> GameEntity::GetMesh() {
	return this->mesh;
}

Transform* GameEntity::GetTransform() {
	return &this->transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial() {
	return this->mat;
}

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2) {
	mat->GetVertShader()->SetShader();

	std::shared_ptr<SimpleVertexShader> vs = mat->GetVertShader();
	vs->SetFloat4("colorTint", mat->GetTint());
	vs->SetMatrix4x4("world", transform.GetWorldMatrix());
	vs->SetMatrix4x4("view", cam->GetViewMatrix());
	vs->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	vs->SetMatrix4x4("lightView", shadowCam1->GetViewMatrix());
	vs->SetMatrix4x4("lightProjection", shadowCam1->GetProjectionMatrix());

	vs->SetMatrix4x4("envLightView", shadowCam2->GetViewMatrix());
	vs->SetMatrix4x4("envLightProjection", shadowCam2->GetProjectionMatrix());

	vs->CopyAllBufferData();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, this->mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(this->mesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);


	// Finally do the actual drawing
	//  - Do this ONCE PER OBJECT you intend to draw
	//  - This will use all of the currently set DirectX "stuff" (shaders, buffers, etc)
	//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
	//     vertices in the currently set VERTEX BUFFER
	context->DrawIndexed(
		this->mesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
		0,     // Offset to the first index we want to use
		0);    // Offset to add to each index when looking up vertices
}

void GameEntity::DrawFromVerts(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<SimpleVertexShader> vs, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2) {
	vs->SetShader();

	vs->SetFloat4("colorTint", DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	vs->SetMatrix4x4("world", transform.GetWorldMatrix());
	vs->SetMatrix4x4("view", cam->GetViewMatrix());
	vs->SetMatrix4x4("projection", cam->GetProjectionMatrix());
	vs->SetMatrix4x4("lightView", shadowCam1->GetViewMatrix());
	vs->SetMatrix4x4("lightProjection", shadowCam1->GetProjectionMatrix());

	vs->SetMatrix4x4("envLightView", shadowCam2->GetViewMatrix());
	vs->SetMatrix4x4("envLightProjection", shadowCam2->GetProjectionMatrix());

	vs->CopyAllBufferData();

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, this->mesh->GetVertexBuffer().GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(this->mesh->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

	context->DrawIndexed(
		this->mesh->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
		0,     // Offset to the first index we want to use
		0);    // Offset to add to each index when looking up vertices
}

std::string GameEntity::GetName() {
	return this->name;
}

void GameEntity::SetName(std::string Name) {
	this->name = Name;
}

void GameEntity::SetEnableDisable(bool value) {
	this->enabled = value;
}

bool GameEntity::GetEnableDisable() {
	return this->enabled;
}