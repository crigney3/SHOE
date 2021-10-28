#pragma once

#include <memory>
#include <DirectXMath.h>
#include <vector>
#include "GameEntity.fwd.h"

class Transform
{
private:
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT4 rotQuat;

	bool isDirty;
	void MarkChildTransformsDirty();

	Transform* parent;
	std::vector<Transform*> children;
	GameEntity* entity;
public:
	Transform();
	Transform(DirectX::XMMATRIX worldIn, DirectX::XMFLOAT3 posIn = DirectX::XMFLOAT3(+0.0f, +0.0f, +0.0f), DirectX::XMFLOAT3 scaleIn = DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4 rotIn = DirectX::XMFLOAT4(+0.0f, +0.0f, +0.0f, +0.0f), Transform* parent = NULL);
	~Transform();

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 pos);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 rot);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 scale);

	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT3 GetForward();

	void MoveAbsolute(float x, float y, float z);
	void MoveRelative(float x, float y, float z); // Move along our "local" axes (respect rotation)
	void Rotate(float pitch, float yaw, float roll);
	void Scale(float x, float y, float z);

	void AddChild(Transform* child);
	void RemoveChild(Transform* child);
	void SetParent(Transform* newParent);
	Transform* GetParent();
	Transform* GetChild(unsigned int index);
	int IndexOfChild(Transform* child);
	unsigned int GetChildCount();

	GameEntity* GetGameEntity();
	void SetGameEntity(GameEntity* entity);
	std::vector<GameEntity*> GetChildrenAsGameEntities();
};

