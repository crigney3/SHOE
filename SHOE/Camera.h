#pragma once

#include "Transform.h"
#include "Input.h"

#include <DirectXMath.h>
#include <Windows.h>
#include <memory>

class Camera
{
public:
	Camera(float x, float y, float z, float aspectRatio, bool type);
	Camera(DirectX::XMFLOAT3 pos, float aspectRatio, bool type);
	~Camera();

	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();
	std::shared_ptr<Transform> GetTransform();

	void UpdateProjectionMatrix(float aspectRatio, bool type);
	void UpdateViewMatrix();
	void Update(float dt, HWND windowHandle);
	void SetFOV(float fov);
	void SetNearDist(float nearDist);
	void SetFarDist(float farDist);
	void SetMoveSpeed(float moveSpeed);
	void SetLookSpeed(float lookSpeed);

private:
	std::shared_ptr<Transform> transform;
	DirectX::XMFLOAT4X4 vMatrix;
	DirectX::XMFLOAT4X4 projMatrix;
	POINT previousMousePoint;

	float fov;
	float nearDist;
	float farDist;
	float moveSpeed;
	float lookSpeed;
	float prevAspectRatio;
	bool type;
};

