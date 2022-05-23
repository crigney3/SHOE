#include "../Headers/Camera.h"
#include "..\Headers\ComponentManager.h"

using namespace DirectX;

void Camera::UpdateViewMatrix()
{
	XMVECTOR forwardDirection = XMLoadFloat3(&GetTransform()->GetForward());
	XMVECTOR position = XMLoadFloat3(&GetTransform()->GetGlobalPosition());

	XMMATRIX view = XMMatrixLookToLH(position, forwardDirection, XMVectorSet(0, 1, 0, 0));	

	XMStoreFloat4x4(&vMatrix, view);
}

void Camera::UpdateProjectionMatrix()
{
	XMMATRIX proj;
	if(isPerspective) proj = XMMatrixPerspectiveFovLH(this->fov, aspectRatio, this->nearDist, this->farDist);
	else proj = XMMatrixOrthographicLH(10.0f, 10.0f, this->nearDist, this->farDist);

	XMStoreFloat4x4(&projMatrix, proj);
}

void Camera::Start()
{
	this->fov = XM_PIDIV4;
	this->farDist = 500.0f;
	this->nearDist = 0.01f;
	this->aspectRatio = 1.0f;
	this->isPerspective = true;

	UpdateViewMatrix();
	UpdateProjectionMatrix();
}

void Camera::OnTransform()
{
	UpdateViewMatrix();
}

void Camera::OnParentTransform(std::shared_ptr<GameEntity> parent)
{
	UpdateViewMatrix();
}

XMFLOAT4X4 Camera::GetViewMatrix()
{
	return vMatrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrix()
{
	return projMatrix;
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

float Camera::GetAspectRatio() {
	return this->aspectRatio;
}

bool Camera::IsPerspective() {
	return this->isPerspective;
}

void Camera::SetFOV(float fov) {
	if (this->fov != fov) {
		this->fov = fov;
		UpdateProjectionMatrix();
	}
}

void Camera::SetNearDist(float nearDist) {
	if (this->nearDist != nearDist) {
		this->nearDist = nearDist;
		UpdateProjectionMatrix();
	}
}

void Camera::SetFarDist(float farDist) {
	if (this->farDist != farDist) {
		this->farDist = farDist;
		UpdateProjectionMatrix();
	}
}

void Camera::SetAspectRatio(float newAspectRatio) {
	if (this->aspectRatio != newAspectRatio) {
		this->aspectRatio = newAspectRatio;
		UpdateProjectionMatrix();
	}
}

void Camera::SetIsPerspective(bool newProjMatrixType) {
	if (this->isPerspective != newProjMatrixType) {
		this->isPerspective = newProjMatrixType;
		UpdateProjectionMatrix();
	}
}
