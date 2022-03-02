#include "../Headers/GameEntity.h"

/**
 * \brief Updates children and attached components with whether the object's parent is enabled
 * \param active Whether this entity's parent is considered enabled
 */
void GameEntity::UpdateHierarchyIsEnabled(bool active)
{
	for (std::shared_ptr<ComponentPacket> packet : componentList)
	{
		packet->component->UpdateHierarchyIsEnabled(GetEnableDisable());
	}
	for (std::shared_ptr<GameEntity> children : transform->GetChildrenAsGameEntities())
	{
		children->UpdateHierarchyIsEnabled(GetEnableDisable());
	}
}

GameEntity::GameEntity(std::shared_ptr<Mesh> mesh, DirectX::XMMATRIX worldIn, std::shared_ptr<Material> mat, std::string name) {
	this->mesh = mesh;
	this->componentList = std::vector<std::shared_ptr<ComponentPacket>>();
	this->mat = mat;
	this->name = name;
	this->enabled = true;
	this->hierarchyIsEnabled = true;
}

GameEntity::~GameEntity() {
}

/**
 * \brief Delays attaching the transform until the self-reference can be made
 * To be called after instantiation
 */
void GameEntity::Initialize()
{
	this->transform = ComponentManager::Instantiate<Transform>(shared_from_this(), this->GetHierarchyIsEnabled());
}

/**
 * \brief Updates all attached components
 */
void GameEntity::Update(float deltaTime, float totalTime)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->Update(deltaTime, totalTime);
		}
	}
}

/**
 * \brief Called on entering a collision with another GameEntity with a collider attached
 * \param other Entity collided with
 */
void GameEntity::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->OnCollisionEnter(other);
		}
	}
}

/**
 * \brief Called on entering another GameEntity's trigger box
 * \param other Entity collided with
 */
void GameEntity::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->OnTriggerEnter(other);
		}
	}
}

/**
 * \brief Delays attaching the transform until the self-reference can be made
 * To be called after instantiation
 */
void GameEntity::Initialize()
{
	this->transform = ComponentManager::Instantiate<Transform>(shared_from_this(), this->GetHierarchyIsEnabled());
}

/**
 * \brief Updates all attached components
 */
void GameEntity::Update(float deltaTime, float totalTime)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->Update(deltaTime, totalTime);
		}
	}
}

/**
 * \brief Called on entering a collision with another GameEntity with a collider attached
 * \param other Entity collided with
 */
void GameEntity::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->OnCollisionEnter(other);
		}
	}
}

/**
 * \brief Called on entering another GameEntity's trigger box
 * \param other Entity collided with
 */
void GameEntity::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
	if (GetEnableDisable()) {
		for (std::shared_ptr<ComponentPacket> packet : componentList) {
			if (packet->component->IsEnabled())
				packet->component->OnTriggerEnter(other);
		}
	}
}

std::shared_ptr<Mesh> GameEntity::GetMesh() {
	return this->mesh;
}

std::shared_ptr<Transform> GameEntity::GetTransform() {
	return transform;
}

std::shared_ptr<Material> GameEntity::GetMaterial() {
	return this->mat;
}

void GameEntity::SetMaterial(std::shared_ptr<Material> newMaterial) {
	this->mat = newMaterial;
}

void GameEntity::SetMesh(std::shared_ptr<Mesh> newMesh) {
	this->mesh = newMesh;
}

void GameEntity::Draw(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context, std::shared_ptr<Camera> cam, std::shared_ptr<Camera> shadowCam1, std::shared_ptr<Camera> shadowCam2) {
	mat->GetVertShader()->SetShader();

	std::shared_ptr<SimpleVertexShader> vs = mat->GetVertShader();
	vs->SetFloat4("colorTint", mat->GetTint());
	vs->SetMatrix4x4("world", transform->GetWorldMatrix());
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
	vs->SetMatrix4x4("world", transform->GetWorldMatrix());
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

/**
 * \brief Special case for transform, cannot have multiple transforms
 * \return This entity's transform
 */
template <>
std::shared_ptr<Transform> GameEntity::AddComponent<Transform>()
{
	//Does nothing, cannot have multiple transforms
	return transform;
}

/**
 * \brief Special case for transform, does nothing since the transform has the same lifetime as the entity
 * \return False
 */
template <>
bool GameEntity::RemoveComponent<Transform>()
{
	return false;
}

/**
 * \brief Special case for transform
 * \return This entity's transform
 */
template <>
std::shared_ptr<Transform> GameEntity::GetComponent<Transform>()
{
	return transform;
}

/**
 * \brief Special case for transform
 * \return This entity's transform
 */
template <>
std::vector<std::shared_ptr<Transform>> GameEntity::GetComponents<Transform>()
{
	return std::vector<std::shared_ptr<Transform>> { transform };
}

/**
 * \brief Frees all of the stored objects in the entity so it can be safely destroyed
 */
void GameEntity::Release()
{
	/*for(std::shared_ptr<GameEntity> child : transform->GetChildrenAsGameEntities())
	{
		child->GetTransform()->SetParent(nullptr);
	}*/
	for (std::shared_ptr<ComponentPacket> packet : componentList) {
		packet->component->OnDestroy();
		packet->deallocator(packet->component);
	}
	ComponentManager::Free<Transform>(transform);
}

std::string GameEntity::GetName() {
	return this->name;
}

void GameEntity::SetName(std::string Name) {
	this->name = Name;
}

void GameEntity::SetEnableDisable(bool value) {
	this->enabled = value;
	UpdateHierarchyIsEnabled(hierarchyIsEnabled);
}

bool GameEntity::GetEnableDisable() {
	return this->enabled && this->hierarchyIsEnabled;
}

bool GameEntity::GetHierarchyIsEnabled()
{
	return hierarchyIsEnabled;
}
