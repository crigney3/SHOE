#include "../Headers/Transform.h"
#include "..\Headers\GameEntity.h"
#include "../Headers/Light.h"

using namespace DirectX;

void Transform::Start() {
	this->parent = nullptr;
	this->isDirty = true;
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

void Transform::OnDestroy()
{
	parent = nullptr;
}
#pragma region Setters
void Transform::SetPosition(float x, float y, float z) {
	SetPosition(XMFLOAT3(x, y, z));
}

void Transform::SetPosition(XMFLOAT3 pos) {
	if (this->pos.x != pos.x || this->pos.y != pos.y || this->pos.z != pos.z) {
		XMFLOAT3 delta = XMFLOAT3(pos.x - this->pos.x, pos.y - this->pos.y, pos.z - this->pos.z);
		this->pos = pos;
		GetGameEntity()->OnMove(delta);
		MarkThisDirty();
	}
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
	SetRotation(XMFLOAT3(pitch, yaw, roll));
}

void Transform::SetRotation(XMFLOAT3 rot) {
	if (this->rotQuat.x != rot.x || this->rotQuat.y != rot.y || this->rotQuat.z != rot.z) {
		XMFLOAT3 delta = XMFLOAT3(rot.x - this->rotQuat.x, rot.y - this->rotQuat.y, rot.z - this->rotQuat.z);
		this->rotQuat = XMFLOAT4(rot.x, rot.y, rot.z, +0.0f);
		GetGameEntity()->OnRotate(delta);
		MarkThisDirty();
	}
}

void Transform::SetScale(float x, float y, float z) {
	SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(XMFLOAT3 scale) {
	if (this->scale.x != scale.x || this->scale.y != scale.y || this->scale.z != scale.z) {
		XMFLOAT3 delta = XMFLOAT3(scale.x - this->scale.x, scale.y - this->scale.y, scale.z - this->scale.z);
		this->scale = scale;
		GetGameEntity()->OnScale(delta);
		MarkThisDirty();
	}
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
	SetPosition(XMFLOAT3(x + this->pos.x, y + this->pos.y, z + this->pos.z));
}

void Transform::Rotate(float pitch, float yaw, float roll) {
	SetRotation(XMFLOAT3(pitch + this->rotQuat.x, yaw + this->rotQuat.y, roll + this->rotQuat.z));
}

void Transform::Scale(float x, float y, float z) {
	SetScale(XMFLOAT3(x * this->scale.x, y * this->scale.y, z * this->scale.z));
}

void Transform::MoveRelative(float x, float y, float z)
{
	if (x == 0 && y == 0 && z == 0) return;
	XMVECTOR desiredMovement = XMVectorSet(x, y, z, 0);

	XMVECTOR rotationQuat =
		XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat4(&rotQuat));

	XMVECTOR relativeMovement = XMVector3Rotate(
		desiredMovement,
		rotationQuat);

	XMFLOAT3 finalPos;
	XMStoreFloat3(&finalPos, XMLoadFloat3(&pos) + relativeMovement);

	SetPosition(finalPos);
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
	if (std::find(children.begin(), children.end(), child) == children.end()) {
		this->children.push_back(child);

		child->parent = shared_from_this();
		child->GetGameEntity()->UpdateHierarchyIsEnabled(GetGameEntity()->GetEnableDisable());
		child->MarkThisDirty();
	}
}

void Transform::RemoveChild(std::shared_ptr<Transform> child) {
	child->GetGameEntity()->UpdateHierarchyIsEnabled(true);
	child->parent = nullptr;
	children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void Transform::SetParent(std::shared_ptr<Transform> newParent) {
    if(parent != newParent){
	    if (newParent.get() != nullptr) {
		    this->parent = newParent;

		    // Ensure internals are correctly updated
		    XMVECTOR childPos = XMLoadFloat3(&pos);
		    XMVECTOR parentPos = XMLoadFloat3(&this->parent->GetGlobalPosition());
		    childPos -= parentPos;

		    XMVECTOR childRot = XMLoadFloat4(&rotQuat);
		    XMVECTOR parentRot = XMLoadFloat4(&this->parent->GetGlobalRotation());
		    childRot -= parentRot;

		    XMVECTOR childScale = XMLoadFloat3(&scale);
		    XMVECTOR parentScale = XMLoadFloat3(&this->parent->GetGlobalScale());
		    childScale /= parentScale;

		    XMStoreFloat3(&this->pos, childPos);
		    XMStoreFloat4(&this->rotQuat, childRot);
		    XMStoreFloat3(&this->scale, childScale);

		    newParent->children.push_back(shared_from_this());
	    }
	    else {
		    // Ensure internals are correctly updated
		    XMVECTOR childPos = XMLoadFloat3(&pos);
		    XMVECTOR parentPos = XMLoadFloat3(&this->parent->GetGlobalPosition());
		    childPos += parentPos;

		    XMVECTOR childRot = XMLoadFloat4(&rotQuat);
		    XMVECTOR parentRot = XMLoadFloat4(&this->parent->GetGlobalRotation());
		    childRot += parentRot;

		    XMVECTOR childScale = XMLoadFloat3(&scale);
		    XMVECTOR parentScale = XMLoadFloat3(&this->parent->GetGlobalScale());
		    childScale *= parentScale;

		    XMStoreFloat3(&this->pos, childPos);
		    XMStoreFloat4(&this->rotQuat, childRot);
		    XMStoreFloat3(&this->scale, childScale);

		    this->parent->RemoveChild(shared_from_this());

		    this->parent = nullptr;
	    }

	    MarkThisDirty();
    }
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