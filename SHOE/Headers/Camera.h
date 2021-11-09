#pragma once

#include "Transform.h"
#include "Input.h"

#include <DirectXMath.h>
#include <Windows.h>
#include <memory>
#include <string>

class Camera
{
public:
	Camera(float x,
		   float y,
		   float z,
		   float aspectRatio,
		   bool type,
		   std::string name = "camera");
	Camera(DirectX::XMFLOAT3 pos,
		   float aspectRatio,
		   bool type,
		   std::string name = "camera");
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

	void SetEnableDisable(bool value);
	bool GetEnableDisable();

	std::string GetName();

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

	bool enabled;
	std::string name;
};

