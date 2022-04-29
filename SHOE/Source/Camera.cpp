#include "../Headers/Camera.h"
#include "..\Headers\ComponentManager.h"
#include "../Headers/Time.h"

using namespace DirectX;

Camera::Camera(float x, float y, float z, float aspectRatio, bool projMatrixType, std::string name, CameraType cameraTag)
{
	this->transform = ComponentManager::Instantiate<Transform>(nullptr);
	transform->SetPosition(x, y, z);

	this->fov = XM_PIDIV4;
	this->farDist = 500.0f;
	this->nearDist = 0.01f;
	this->lookSpeed = 3.0f;
	this->moveSpeed = 10.0f;
	this->enabled = true;
	this->name = name;
	this->tag = cameraTag;

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio, projMatrixType);
}

Camera::Camera(DirectX::XMFLOAT3 pos, float aspectRatio, bool projMatrixType, std::string name, CameraType cameraTag)
{
	this->transform = ComponentManager::Instantiate<Transform>(nullptr);
	transform->SetPosition(pos);

	this->fov = XM_PIDIV4;
	this->farDist = 500.0f;
	this->nearDist = 0.01f;
	this->lookSpeed = 3.0f;
	this->moveSpeed = 10.0f;
	this->enabled = true;
	this->name = name;
	this->tag = cameraTag;

	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio, projMatrixType);
}

Camera::~Camera()
{
	transform->OnDestroy();
	ComponentManager::Free<Transform>(transform);
}

void Camera::Update(HWND windowHandle)
{
	float speed = Time::deltaTime * moveSpeed;

	Input& input = Input::GetInstance();

	// Speed up or down as necessary
	if (input.KeyDown(VK_SHIFT)) { speed *= 5; }
	if (input.KeyDown(VK_CONTROL)) { speed *= 0.1f; }

	transform->MoveRelative(input.TestInputAxis(InputAxes::MovementStrafe) * speed, 0, input.TestInputAxis(InputAxes::MovementAdvance) * speed);
	transform->MoveAbsolute(0, input.TestInputAxis(InputAxes::MovementY) * speed, 0);

	POINT mousePos = {};
	GetCursorPos(&mousePos);
	ScreenToClient(windowHandle, &mousePos);

	if (input.MouseLeftDown())
	{
		float xDiff = this->lookSpeed * Time::deltaTime * input.GetMouseXDelta();
		float yDiff = this->lookSpeed * Time::deltaTime * input.GetMouseYDelta();

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

void Camera::UpdateProjectionMatrix(float aspectRatio, bool projMatrixType)
{
	XMMATRIX proj;
	if(projMatrixType) proj = XMMatrixPerspectiveFovLH(this->fov, aspectRatio, this->nearDist, this->farDist);
	else proj = XMMatrixOrthographicLH(10.0f, 10.0f, this->nearDist, this->farDist);

	XMStoreFloat4x4(&projMatrix, proj);

	this->aspectRatio = aspectRatio;
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

	UpdateProjectionMatrix(aspectRatio, this->projMatrixType);
}

void Camera::SetLookSpeed(float lookSpeed) {
	this->lookSpeed = lookSpeed;
}

void Camera::SetMoveSpeed(float moveSpeed) {
	this->moveSpeed = moveSpeed;
}

void Camera::SetNearDist(float nearDist) {
	this->nearDist = nearDist;

	UpdateProjectionMatrix(aspectRatio, this->projMatrixType);
}

void Camera::SetFarDist(float farDist) {
	this->farDist = farDist;

	UpdateProjectionMatrix(aspectRatio, this->projMatrixType);
}

float Camera::GetAspectRatio() {
	return this->aspectRatio;
}

void Camera::SetAspectRatio(float newAspectRatio) {
	UpdateProjectionMatrix(newAspectRatio, this->projMatrixType);
}

bool Camera::GetProjectionMatrixType() {
	return this->projMatrixType;
}

void Camera::SetProjectionMatrixType(bool newProjMatrixType) {
	this->projMatrixType = newProjMatrixType;

	UpdateProjectionMatrix(this->aspectRatio, this->projMatrixType);
}

void Camera::SetEnableDisable(bool value) {
	this->enabled = value;
}

bool Camera::GetEnableDisable() {
	return this->enabled;
}

CameraType Camera::GetTag() {
	return this->tag;
}

/// <summary>
/// Be wary of calling this directly, as giving multiple cameras the 
/// Main or Play tags results in undefined behavior! Prefer
///	AssetManager's SetCameraTag, which handles these cases.
/// </summary>
/// <param name="tag"></param>
void Camera::SetTag(CameraType tag) {
	this->tag = tag;
}
