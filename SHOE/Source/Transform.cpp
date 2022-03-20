#include "../Headers/Transform.h"

using namespace DirectX;

void Transform::Start() {
	this->parent = nullptr;
	this->isDirty = false;
	XMStoreFloat4x4(&this->worldMatrix, XMMatrixIdentity());
	this->pos = XMFLOAT3(+0.0f, +0.0f, +0.0f);
	this->scale = XMFLOAT3(+1.0f, +1.0f, +1.0f);
	this->rotQuat = XMFLOAT4(+0.0f, +0.0f, +0.0f, +0.0f);
	this->children.clear();
}

void Transform::UpdateWorldInfo()
{
	if (!this->isDirty) return;

	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX pWorld = XMMatrixIdentity();
	XMFLOAT4X4 parentWorldF = XMFLOAT4X4();

	// - World Matrix - //
	{
		XMMATRIX trans = XMMatrixTranslation(this->pos.x, this->pos.y, this->pos.z);
		XMMATRIX scale = XMMatrixScaling(this->scale.x, this->scale.y, this->scale.z);
		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(this->rotQuat.x, this->rotQuat.y, this->rotQuat.z);

		world = scale * rotation * trans;

		if (parent != nullptr)
		{
			parentWorldF = parent->GetWorldMatrix();
			world = XMMatrixMultiply(world, XMLoadFloat4x4(&parentWorldF));
		}

		// Store in field
		XMStoreFloat4x4(&this->worldMatrix, world);
	}

	// - Update stored global Position, Rotation, & Scale - //
	if (parent != nullptr)
	{
		XMVECTOR posVec, sclVec, rotVec;

		XMMatrixDecompose(&sclVec, &rotVec, &posVec, world);

		// Store in field
		XMStoreFloat3(&worldPos, posVec);
		XMStoreFloat3(&worldScale, sclVec);
		XMStoreFloat4(&worldRotQuat, rotVec);
	}

	// Reset the bool
	this->isDirty = false;
}
#pragma region Setters
void Transform::SetPosition(float x, float y, float z) {
	SetPosition(XMFLOAT3(x, y, z));
	MarkThisDirty();
}

void Transform::SetPosition(XMFLOAT3 pos) {
	this->pos = pos;
	MarkThisDirty();
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
	SetRotation(XMFLOAT3(pitch, yaw, roll));
	MarkThisDirty();
}

void Transform::SetRotation(XMFLOAT3 rot) {
	this->rotQuat = XMFLOAT4(rot.x, rot.y, rot.z, +0.0f);
	MarkThisDirty();
}

void Transform::SetScale(float x, float y, float z) {
	SetScale(XMFLOAT3(x, y, z));
	MarkThisDirty();
}

void Transform::SetScale(XMFLOAT3 scale) {
	this->scale = scale;
	MarkThisDirty();
}
# pragma endregion

#pragma region Getters
XMFLOAT3 Transform::GetLocalPosition() {
	return this->pos;
}

DirectX::XMFLOAT3 Transform::GetGlobalPosition()
{
	// Make sure world info is updated if necessary
	if (this->isDirty) UpdateWorldInfo();

	if (parent == nullptr) return this->pos;
	return worldPos;
}

XMFLOAT3 Transform::GetLocalPitchYawRoll() {
	return XMFLOAT3(this->rotQuat.x, this->rotQuat.y, this->rotQuat.z);
}

DirectX::XMFLOAT4 Transform::GetGlobalRotation()
{
	// Make sure world info is updated if necessary
	if (this->isDirty) UpdateWorldInfo();

	return worldRotQuat;
}

XMFLOAT3 Transform::GetLocalScale() {
	return this->scale;
}

DirectX::XMFLOAT3 Transform::GetGlobalScale()
{
	// Make sure world info is updated if necessary
	if (this->isDirty) UpdateWorldInfo();

	if (parent == nullptr) return scale;
	return worldScale;
}

XMFLOAT4X4 Transform::GetWorldMatrix()
{
	// Make sure world info is updated if necessary
	if (this->isDirty) UpdateWorldInfo();

	return this->worldMatrix;
}

bool Transform::GetDirtyStatus() { return isDirty; }
#pragma endregion

#pragma region Transformation Methods
void Transform::MoveAbsolute(float x, float y, float z) {
	float newX = x + this->pos.x;
	float newY = y + this->pos.y;
	float newZ = z + this->pos.z;

	this->pos = XMFLOAT3(newX, newY, newZ);

	MarkThisDirty();
}

void Transform::Rotate(float pitch, float yaw, float roll) {
	float newPitch = pitch + this->rotQuat.x;
	float newYaw = yaw + this->rotQuat.y;
	float newRoll = roll + this->rotQuat.z;

	this->rotQuat = XMFLOAT4(newPitch, newYaw, newRoll, +0.0f);
	MarkThisDirty();
}

void Transform::Scale(float x, float y, float z) {
	float newX = x * this->scale.x;
	float newY = y * this->scale.y;
	float newZ = z * this->scale.z;

	this->scale = XMFLOAT3(newX, newY, newZ);

	MarkThisDirty();
}

void Transform::MoveRelative(float x, float y, float z)
{
	XMVECTOR desiredMovement = XMVectorSet(x, y, z, 0);

	XMVECTOR rotationQuat =
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat4(&rotQuat));

	XMVECTOR relativeMovement = XMVector3Rotate(
		desiredMovement,
		rotationQuat);

	XMStoreFloat3(&pos, XMLoadFloat3(&pos) + relativeMovement);

	MarkThisDirty();
}
#pragma endregion

DirectX::XMFLOAT3 Transform::GetForward() {
	XMFLOAT3 forwardVec;
	XMVECTOR desiredMovement = XMVectorSet(0, 0, 1, 0);

	XMVECTOR rotationQuat =
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat4(&rotQuat));

	XMVECTOR relativeMovement = XMVector3Rotate(
		desiredMovement,
		rotationQuat);

	XMStoreFloat3(&forwardVec, relativeMovement);

	return forwardVec;
}

void Transform::AddChild(std::shared_ptr<Transform> child) {
	this->children.push_back(child);

	child->parent = shared_from_this();
	child->MarkThisDirty();
}

void Transform::RemoveChild(std::shared_ptr<Transform> child) {
	child->parent = nullptr;
	children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void Transform::SetParent(std::shared_ptr<Transform> newParent) {
	if (newParent.get() != nullptr) {
		this->parent = newParent;
		newParent->children.push_back(shared_from_this());
	}
	else {
		this->parent->RemoveChild(shared_from_this());
		this->parent = nullptr;
	}

	MarkThisDirty();
}

std::shared_ptr<Transform> Transform::GetParent() {
	return parent;
}

std::shared_ptr<Transform> Transform::GetChild(unsigned int index) {
	if (children[index] != NULL) {
		return children[index];
	}
	return 0;
}

unsigned int Transform::GetChildCount() {
	return children.size();
}

void Transform::MarkThisDirty()
{
	this->isDirty = true;
	MarkChildTransformsDirty();
}

void Transform::MarkChildTransformsDirty() {
	for (int i = 0; i < children.size(); i++) {
		children[i]->MarkThisDirty();
	}
}

std::vector<std::shared_ptr<GameEntity>> Transform::GetChildrenAsGameEntities() {
	std::vector<std::shared_ptr<GameEntity>> list = std::vector<std::shared_ptr<GameEntity>>();

	for (unsigned int i = 0; i < GetChildCount(); i++) {
		list.push_back(children[i]->GetGameEntity());
	}

	return list;
}