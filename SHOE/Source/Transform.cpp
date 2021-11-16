#include "../Headers/Transform.h"

using namespace DirectX;

Transform::Transform() {
	this->parent = NULL;
	this->isDirty = false;
	XMStoreFloat4x4(&this->worldMatrix, XMMatrixIdentity());
	this->pos = XMFLOAT3(+0.0f, +0.0f, +0.0f);
	this->scale = XMFLOAT3(+1.0f, +1.0f, +1.0f);
	this->rotQuat = XMFLOAT4(+0.0f, +0.0f, +0.0f, +0.0f);
	this->children = std::vector<Transform*>();
	this->entity = NULL;
}

Transform::Transform(XMMATRIX worldIn, XMFLOAT3 posIn, XMFLOAT3 scaleIn, XMFLOAT4 rotIn, Transform* parent) {
	this->parent = parent;
	this->isDirty = false;
	XMStoreFloat4x4(&this->worldMatrix, worldIn);
	this->pos = posIn;
	this->scale = scaleIn;
	this->rotQuat = rotIn;
	this->children = std::vector<Transform*>();
	this->entity = NULL;
}

Transform::~Transform() {
	//delete &this->worldMatrix;
}

void Transform::SetPosition(float x, float y, float z) {
	isDirty = true;
	this->pos = XMFLOAT3(x, y, z);
}

void Transform::SetPosition(XMFLOAT3 pos) {
	isDirty = true;
	this->pos = pos;
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
	isDirty = true;
	this->rotQuat = XMFLOAT4(pitch, yaw, roll, +0.0f);
}

void Transform::SetRotation(XMFLOAT3 rot) {
	isDirty = true;
	this->rotQuat = XMFLOAT4(rot.x, rot.y, rot.z, +0.0f);
}

void Transform::SetScale(float x, float y, float z) {
	isDirty = true;
	this->scale = XMFLOAT3(x, y, z);
}

void Transform::SetScale(XMFLOAT3 scale) {
	isDirty = true;
	this->scale = scale;
}

XMFLOAT3 Transform::GetPosition() {
	return this->pos;
}

XMFLOAT3 Transform::GetPitchYawRoll() {
	return XMFLOAT3(this->rotQuat.x, this->rotQuat.y, this->rotQuat.z);
}

XMFLOAT3 Transform::GetScale() {
	return this->scale;
}

XMFLOAT4X4 Transform::GetWorldMatrix() {
	if (this->isDirty) {
		XMMATRIX trans = XMMatrixTranslation(this->pos.x, this->pos.y, this->pos.z);
		XMMATRIX scale = XMMatrixScaling(this->scale.x, this->scale.y, this->scale.z);
		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(this->rotQuat.x, this->rotQuat.y, this->rotQuat.z);

		XMMATRIX world = scale * rotation * trans;

		if (parent != NULL) {
			XMFLOAT4X4 parentWorld = parent->GetWorldMatrix();
			world = XMMatrixMultiply(world, XMLoadFloat4x4(&parentWorld));
		}

		XMStoreFloat4x4(&this->worldMatrix, world);

		this->isDirty = false;

		return this->worldMatrix;
	}
	else {
		return this->worldMatrix;
	}
}

void Transform::MoveAbsolute(float x, float y, float z) {
	float newX = x + this->pos.x;
	float newY = y + this->pos.y;
	float newZ = z + this->pos.z;

	this->pos = XMFLOAT3(newX, newY, newZ);

	isDirty = true;
}

void Transform::Rotate(float pitch, float yaw, float roll) {
	float newPitch = pitch + this->rotQuat.x;
	float newYaw = yaw + this->rotQuat.y;
	float newRoll = roll + this->rotQuat.z;

	this->rotQuat = XMFLOAT4(newPitch, newYaw, newRoll, +0.0f);

	isDirty = true;
}

void Transform::Scale(float x, float y, float z) {
	float newX = x * this->scale.x;
	float newY = y * this->scale.y;
	float newZ = z * this->scale.z;

	this->scale = XMFLOAT3(newX, newY, newZ);

	isDirty = true;
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

	isDirty = true;
}

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

void Transform::AddChild(Transform* child) {
	this->children.push_back(child);

	child->isDirty = true;
	child->MarkChildTransformsDirty();
	child->parent = this;
}

void Transform::RemoveChild(Transform* child) {
	int index = IndexOfChild(child);
	if (index != -1) {

		children[index] = NULL;
	}
}

void Transform::SetParent(Transform* newParent) {
	if (newParent != NULL) {
		this->parent = newParent;
		newParent->children.push_back(this);
	}
	else {
		this->parent = NULL;
	}

	isDirty = true;
	MarkChildTransformsDirty();
}

Transform* Transform::GetParent() {
	return parent;
}

Transform* Transform::GetChild(unsigned int index) {
	if (children[index] != NULL) {
		return children[index];
	}
	return 0;
}

int Transform::IndexOfChild(Transform* child) {
	for (int i = 0; i < children.size(); i++) {
		if (children[i] == child) {
			return i;
		}
	}
	return -1;
}

unsigned int Transform::GetChildCount() {
	return children.size();
}

void Transform::MarkChildTransformsDirty() {
	for (int i = 0; i < children.size(); i++) {
		children[i]->isDirty = true;
		children[i]->MarkChildTransformsDirty();
	}
}

GameEntity* Transform::GetGameEntity() {
	return this->entity;
}

void Transform::SetGameEntity(GameEntity* entity) {
	this->entity = entity;
}

std::vector<GameEntity*> Transform::GetChildrenAsGameEntities() {
	std::vector<GameEntity*> list = std::vector<GameEntity*>();

	for (unsigned int i = 0; i < GetChildCount(); i++) {
		list.push_back(children[i]->GetGameEntity());
	}

	return list;
}