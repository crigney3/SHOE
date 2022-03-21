#include "../Headers/Camera.h"
#include "..\Headers\ComponentManager.h"

using namespace DirectX;

Camera::Camera(float x, float y, float z, float aspectRatio, bool type, std::string name)
{
	this->transform = ComponentManager::Instantiate<Transform>(nullptr, true);
	transform->SetPosition(x, y, z);

	this->fov = XM_PIDIV4;
	this->farDist = 500.0f;
	this->nearDist = 0.01f;
	this->lookSpeed = 3.0f;
	this->moveSpeed = 10.0f;
	this->enabled = true;
	this->name = name;

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio, type);
}

Camera::Camera(DirectX::XMFLOAT3 pos, float aspectRatio, bool type, std::string name)
{
	this->transform = ComponentManager::Instantiate<Transform>(nullptr, true);
	transform->SetPosition(pos);

	this->fov = XM_PIDIV4;
	this->farDist = 500.0f;
	this->nearDist = 0.01f;
	this->lookSpeed = 3.0f;
	this->moveSpeed = 10.0f;
	this->enabled = true;
	this->name = name;

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio, type);
}

Camera::~Camera()
{
	transform->OnDestroy();
	ComponentManager::Free<Transform>(transform);
}

void Camera::Update(float dt, HWND windowHandle)
{
	float speed = dt * moveSpeed;

	Input& input = Input::GetInstance();

	// Speed up or down as necessary
	if (input.KeyDown(VK_SHIFT)) { speed *= 5; }
	if (input.KeyDown(VK_CONTROL)) { speed *= 0.1f; }

	// Movement
	if (input.TestKeyAction(KeyActions::MoveForward)) { transform->MoveRelative(0, 0, speed); }
	if (input.TestKeyAction(KeyActions::MoveBack)) { transform->MoveRelative(0, 0, -speed); }
	if (input.TestKeyAction(KeyActions::StrafeLeft)) { transform->MoveRelative(-speed, 0, 0); }
	if (input.TestKeyAction(KeyActions::StrafeRight)) { transform->MoveRelative(speed, 0, 0); }
	if (input.TestKeyAction(KeyActions::MoveDown)) { transform->MoveAbsolute(0, -speed, 0); }
	if (input.TestKeyAction(KeyActions::MoveUp)) { transform->MoveAbsolute(0, speed, 0); }

	POINT mousePos = {};
	GetCursorPos(&mousePos);
	ScreenToClient(windowHandle, &mousePos);

	if (input.MouseLeftDown())
	{
		float xDiff = this->lookSpeed * dt * input.GetMouseXDelta();
		float yDiff = this->lookSpeed * dt * input.GetMouseYDelta();

		//TODO: Fix gimbal lock
		transform->Rotate(yDiff, xDiff, 0);
		/*if (transform->GetLocalPitchYawRoll().y >= XM_PIDIV2) {
			transform->SetRotation(transform->GetLocalPitchYawRoll().x, XM_PIDIV2 - 0.007f, transform->GetLocalPitchYawRoll().z);
		}
		else if (transform->GetLocalPitchYawRoll().y <= -XM_PIDIV2) {
			transform->SetRotation(transform->GetLocalPitchYawRoll().x, -XM_PIDIV2 + 0.007f, transform->GetLocalPitchYawRoll().z);
		}*/
	}

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	DirectX::XMFLOAT4X4 worldNew = transform->GetWorldMatrix();

	XMFLOAT3 pitchYawRollValues = transform->GetLocalPitchYawRoll();

	XMVECTOR forwardDirection = 
		XMVector3Rotate(XMVectorSet(0, 0, 1, 0), XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRollValues)));

	XMFLOAT3 position = transform->GetLocalPosition();
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&position), forwardDirection,	XMVectorSet(0, 1, 0, 0));	

	XMStoreFloat4x4(&vMatrix, view);
}

void Camera::UpdateProjectionMatrix(float aspectRatio, bool type)
{
	XMMATRIX proj;
	if(type) proj = XMMatrixPerspectiveFovLH(this->fov, aspectRatio, this->nearDist, this->farDist);
	else proj = XMMatrixOrthographicLH(10.0f, 10.0f, this->nearDist, this->farDist);

	XMStoreFloat4x4(&projMatrix, proj);

	this->prevAspectRatio = aspectRatio;
}

std::string Camera::GetName() {
	return this->name;
}

void Camera::SetName(std::string name) {
	this->name = name;
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	return vMatrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return projMatrix;
}

std::shared_ptr<Transform> Camera::GetTransform() {
	return this->transform;
}

float Camera::GetFOV() {
	return this->fov;
}

float Camera::GetNearDist() {
	return this->nearDist;
}

float Camera::GetFarDist() {
	return this->farDist;
}

float Camera::GetLookSpeed() {
	return this->lookSpeed;
}

float Camera::GetMoveSpeed() {
	return this->moveSpeed;
}

void Camera::SetFOV(float fov) {
	this->fov = fov;

	UpdateProjectionMatrix(prevAspectRatio, this->type);
}

void Camera::SetLookSpeed(float lookSpeed) {
	this->lookSpeed = lookSpeed;
}

void Camera::SetMoveSpeed(float moveSpeed) {
	this->moveSpeed = moveSpeed;
}

void Camera::SetNearDist(float nearDist) {
	this->nearDist = nearDist;

	UpdateProjectionMatrix(prevAspectRatio, this->type);
}

void Camera::SetFarDist(float farDist) {
	this->farDist = farDist;

	UpdateProjectionMatrix(prevAspectRatio, this->type);
}

void Camera::SetEnableDisable(bool value) {
	this->enabled = value;
}

bool Camera::GetEnableDisable() {
	return this->enabled;
}
