#pragma once

#include "Transform.h"
#include "Input.h"

#include <DirectXMath.h>
#include <Windows.h>
#include <string>

class Camera : public IComponent
{
public:
	DirectX::XMFLOAT4X4 GetViewMatrix();
	DirectX::XMFLOAT4X4 GetProjectionMatrix();

	void UpdateProjectionMatrix();
	void UpdateViewMatrix();

	float GetFOV();
	void SetFOV(float fov);

	float GetNearDist();
	void SetNearDist(float nearDist);

	float GetFarDist();
	void SetFarDist(float farDist);

	float GetAspectRatio();
	void SetAspectRatio(float newAspectRatio);

	bool GetProjectionMatrixType();
	void SetProjectionMatrixType(bool newProjMatrixType);
protected:
	void Start() override;
private:
	DirectX::XMFLOAT4X4 vMatrix;
	DirectX::XMFLOAT4X4 projMatrix;

	float fov;
	float nearDist;
	float farDist;
	float aspectRatio;
	bool projMatrixType;

	void OnTransform() override;
	void OnParentTransform(std::shared_ptr<GameEntity> parent) override;
};

