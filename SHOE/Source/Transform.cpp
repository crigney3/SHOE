#include "../Headers/Transform.h"
#include "..\Headers\GameEntity.h"
#include "../Headers/Light.h"

using namespace DirectX;

void Transform::Start() {
	this->parent = nullptr;
	this->matricesDirty = false;
	this->vectorsDirty = false;
	this->globalsDirty = true;

	this->position = XMFLOAT3(0, 0, 0);
	this->pitchYawRoll = XMFLOAT3(0, 0, 0);
	this->scale = XMFLOAT3(1, 1, 1);

	this->up = XMFLOAT3(0, 1, 0);
	this->right = XMFLOAT3(1, 0, 0);
	this->forward = XMFLOAT3(0, 0, 1);

	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixIdentity());
	this->children.clear();
	this->childEntities.clear();
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
	if (position.x != pos.x || position.y != pos.y || position.z != pos.z) {
		std::shared_ptr<XMFLOAT3> delta = std::make_shared<XMFLOAT3>(pos.x - position.x, pos.y - position.y, pos.z - position.z);
		position = pos;
		if (GetGameEntity() != nullptr) GetGameEntity()->PropagateEvent(EntityEventType::OnMove, delta);
		MarkMatricesDirty();
	}
}

void Transform::SetRotation(float pitch, float yaw, float roll) {
	SetRotation(XMFLOAT3(pitch, yaw, roll));
}

void Transform::SetRotation(XMFLOAT3 rot) {
	if (pitchYawRoll.x != rot.x || pitchYawRoll.y != rot.y || pitchYawRoll.z != rot.z) {
		std::shared_ptr<XMFLOAT3> delta = std::make_shared<XMFLOAT3>(rot.x - pitchYawRoll.x, rot.y - pitchYawRoll.y, rot.z - pitchYawRoll.z);
		pitchYawRoll = XMFLOAT3(rot.x, rot.y, rot.z);
		if (GetGameEntity() != nullptr) GetGameEntity()->PropagateEvent(EntityEventType::OnRotate, delta);
		MarkMatricesDirty();
		MarkVectorsDirty();
	}
}

void Transform::SetScale(float x, float y, float z) {
	SetScale(XMFLOAT3(x, y, z));
}

void Transform::SetScale(XMFLOAT3 scale) {
	if (this->scale.x != scale.x || this->scale.y != scale.y || this->scale.z != scale.z) {
		std::shared_ptr<XMFLOAT3> delta = std::make_shared<XMFLOAT3>(scale.x - this->scale.x, scale.y - this->scale.y, scale.z - this->scale.z);
		this->scale = scale;
		if (GetGameEntity() != nullptr) GetGameEntity()->PropagateEvent(EntityEventType::OnScale, delta);
		MarkMatricesDirty();
	}
}
# pragma endregion

#pragma region Getters
XMFLOAT3 Transform::GetLocalPosition() {
	return position;
}

DirectX::XMFLOAT3 Transform::GetGlobalPosition()
{
	UpdateGlobals();
	return worldPos;
}

XMFLOAT3 Transform::GetLocalPitchYawRoll() {
	return pitchYawRoll;
}

DirectX::XMFLOAT4 Transform::GetGlobalRotation()
{
	UpdateGlobals();
	return worldRotQuat;
}

XMFLOAT3 Transform::GetLocalScale() {
	return this->scale;
}

DirectX::XMFLOAT3 Transform::GetGlobalScale()
{
	UpdateGlobals();
	return worldScale;
}

XMFLOAT3 Transform::GetUp()
{
	UpdateVectors();
	return up;
}

XMFLOAT3 Transform::GetRight()
{
	UpdateVectors();
	return right;
}

XMFLOAT3 Transform::GetForward()
{
	UpdateVectors();
	return forward;
}


XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateMatrices();
	return this->worldMatrix;
}

XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	UpdateMatrices();
	return worldMatrix;
}

void Transform::MarkMatricesDirty()
{
	matricesDirty = true;
	MarkGlobalsDirty();
}

void Transform::MarkVectorsDirty()
{
	vectorsDirty = true;
}

void Transform::MarkGlobalsDirty()
{
	globalsDirty = true;
	if (parent != nullptr) parent->MarkGlobalsDirty();
}

#pragma endregion

#pragma region Transformation Methods
void Transform::MoveAbsolute(float x, float y, float z) {
	SetPosition(XMFLOAT3(x + position.x, y + position.y, z + position.z));
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
	MoveAbsolute(offset.x, offset.y, offset.z);
}

void Transform::Rotate(float pitch, float yaw, float roll) {
	SetRotation(XMFLOAT3(pitch + pitchYawRoll.x, yaw + pitchYawRoll.y, roll + pitchYawRoll.z));
}

void Transform::Rotate(DirectX::XMFLOAT3 pitchYawRoll)
{
	Rotate(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
}

void Transform::Scale(float x, float y, float z) {
	SetScale(XMFLOAT3(x * scale.x, y * scale.y, z * scale.z));
}

void Transform::Scale(DirectX::XMFLOAT3 scale)
{
	Scale(scale.x, scale.y, scale.z);
}

void Transform::MoveRelative(float x, float y, float z)
{
	if (x == 0 && y == 0 && z == 0) return;
	XMVECTOR desiredMovement = XMVectorSet(x, y, z, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	XMVECTOR dir = XMVector3Rotate(desiredMovement, rotQuat);

	XMFLOAT3 finalPos;
	XMStoreFloat3(&finalPos, XMLoadFloat3(&position) + dir);

	SetPosition(finalPos);
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
	MoveRelative(offset.x, offset.y, offset.z);
}
#pragma endregion

void Transform::UpdateMatrices()
{
	if (!matricesDirty) return;

	// Create the three transformation pieces
	XMMATRIX trans = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
	XMMATRIX rot = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
	XMMATRIX sc = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

	// Combine and store the world
	XMMATRIX wm = sc * rot * trans;

	if (parent != nullptr)
	{
		XMFLOAT4X4 parentWorld = parent->GetWorldMatrix();
		wm *= XMLoadFloat4x4(&parentWorld);
	}

	// Store both versions
	XMStoreFloat4x4(&worldMatrix, wm);
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixInverse(0, XMMatrixTranspose(wm)));

	// Matrices are up to date
	matricesDirty = false;
}

void Transform::UpdateVectors()
{
	if (!vectorsDirty) return;

	// Update all three vectors
	XMVECTOR rotationQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
	XMStoreFloat3(&up, XMVector3Rotate(XMVectorSet(0, 1, 0, 0), rotationQuat));
	XMStoreFloat3(&right, XMVector3Rotate(XMVectorSet(1, 0, 0, 0), rotationQuat));
	XMStoreFloat3(&forward, XMVector3Rotate(XMVectorSet(0, 0, 1, 0), rotationQuat));

	// Vectors are up to date
	vectorsDirty = false;
}

void Transform::UpdateGlobals()
{
	if (!globalsDirty) return;

	XMFLOAT4X4 wM = GetWorldMatrix();

	// Decompose the matrix
	XMVECTOR globalPos;
	XMVECTOR globalRotQuat;
	XMVECTOR globalScale;
	XMMatrixDecompose(&globalScale, &globalRotQuat, &globalPos, XMLoadFloat4x4(&wM));

	XMStoreFloat3(&worldPos, globalPos);
	XMStoreFloat4(&worldRotQuat, globalRotQuat);
	XMStoreFloat3(&worldScale, globalScale);
}

void Transform::AddChild(std::shared_ptr<Transform> child) {
	if (child == nullptr) return;
	if (std::find(children.begin(), children.end(), child) == children.end()) {
		XMFLOAT4X4 parentWorld = GetWorldMatrix();
		XMMATRIX pWorld = XMLoadFloat4x4(&parentWorld);

		XMFLOAT4X4 childWorld = child->GetWorldMatrix();
		XMMATRIX cWorld = XMLoadFloat4x4(&childWorld);

		// Invert the parent
		XMMATRIX pWorldInv = XMMatrixInverse(0, pWorld);

		// Multiply the child by the inverse parent
		XMMATRIX relCWorld = cWorld * pWorldInv;

		// Set the child's transform from this new matrix
		XMFLOAT4X4 relativeChildWorld;
		XMStoreFloat4x4(&relativeChildWorld, relCWorld);
		child->SetTransformsFromMatrix(relativeChildWorld);

		children.push_back(child);
		childEntities.push_back(child->GetGameEntity());
		child->parent = shared_from_this();

		child->GetGameEntity()->UpdateHierarchyIsEnabled(GetGameEntity()->GetEnableDisable());
	}
}

void Transform::RemoveChild(std::shared_ptr<Transform> child) {
	if (child == nullptr) return;
	for (int i = 0; i < children.size(); i++) {
		if (children[i] == child) {
			// Set the child's transform data using its final matrix
			child->SetTransformsFromMatrix(child->GetWorldMatrix());

			children.erase(children.begin() + i);
			childEntities.erase(childEntities.begin() + i);
			child->parent = nullptr;

			child->GetGameEntity()->UpdateHierarchyIsEnabled(true);
			return;
		}
	}
}

void Transform::SetParent(std::shared_ptr<Transform> newParent) {
	if (parent != newParent) {
		if (parent != nullptr) {
			parent->RemoveChild(shared_from_this());
		}
		if (newParent != nullptr) {
			newParent->AddChild(shared_from_this());
		}
	}
}

std::shared_ptr<Transform> Transform::GetParent() {
	return parent;
}

std::shared_ptr<Transform> Transform::GetChild(unsigned int index) {
	if (index < children.size() && children[index] != nullptr) {
		return children[index];
	}
	return nullptr;
}

unsigned int Transform::GetChildCount() {
	return children.size();
}

std::vector<std::shared_ptr<GameEntity>> Transform::GetChildrenEntities() {
	return childEntities;
}

void Transform::SetTransformsFromMatrix(DirectX::XMFLOAT4X4 worldMatrix)
{
	// Decompose the matrix
	XMVECTOR localPos;
	XMVECTOR localRotQuat;
	XMVECTOR localScale;
	XMMatrixDecompose(&localScale, &localRotQuat, &localPos, XMLoadFloat4x4(&worldMatrix));

	// Get the euler angles from the quaternion and store as our 
	XMFLOAT4 quat;
	XMStoreFloat4(&quat, localRotQuat);
	pitchYawRoll = QuaternionToEuler(quat);

	// Overwrite the child's other transform data
	XMStoreFloat3(&position, localPos);
	XMStoreFloat3(&scale, localScale);

	// Things have changed
	matricesDirty = true;
	vectorsDirty = true;
}

DirectX::XMFLOAT3 Transform::QuaternionToEuler(DirectX::XMFLOAT4 quaternion)
{
	// Convert quaternion to euler angles
	// Note: This will give a set of euler angles, but not necessarily
	// the same angles that were used to create the quaternion

	// Step 1: Quaternion to rotation matrix
	XMMATRIX rMat = XMMatrixRotationQuaternion(XMLoadFloat4(&quaternion));

	// Step 2: Extract each piece
	// From: https://stackoverflow.com/questions/60350349/directx-get-pitch-yaw-roll-from-xmmatrix
	XMFLOAT4X4 rotationMatrix;
	XMStoreFloat4x4(&rotationMatrix, rMat);
	float pitch = (float)asin(-rotationMatrix._32);
	float yaw = (float)atan2(rotationMatrix._31, rotationMatrix._33);
	float roll = (float)atan2(rotationMatrix._12, rotationMatrix._22);

	// Return the euler values as a vector
	return XMFLOAT3(pitch, yaw, roll);
}