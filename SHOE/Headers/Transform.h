#pragma once

#include <DirectXMath.h>
#include <vector>
#include "GameEntity.fwd.h"
#include "IComponent.h"

class Transform : public IComponent,  public std::enable_shared_from_this<Transform>
{
private:
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT3 worldPos;
	DirectX::XMFLOAT3 worldScale;

	DirectX::XMFLOAT3 pos; //LOCAL SPACE
	DirectX::XMFLOAT3 scale; //LOCAL SPACE
	DirectX::XMFLOAT4 rotQuat; //LOCAL SPACE(?)

	bool isDirty; // Can/will only be dirty if: has a parent & parent has moved
	void MarkThisDirty();
	void MarkChildTransformsDirty();

	std::shared_ptr<Transform> parent;
	std::vector<std::shared_ptr<Transform>> children = std::vector<std::shared_ptr<Transform>>();
public:
	void Start() override;
	void UpdateWorldInfo();

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 pos);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 rot);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 scale);

	DirectX::XMFLOAT3 GetLocalPosition();
	DirectX::XMFLOAT3 GetGlobalPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetLocalScale();
	DirectX::XMFLOAT3 GetGlobalScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT3 GetForward();

	DirectX::XMFLOAT3 GetXAxis();
	DirectX::XMFLOAT3 GetYAxis();
	DirectX::XMFLOAT3 GetZAxis();

	bool GetDirtyStatus();

	void MoveAbsolute(float x, float y, float z);
	void MoveRelative(float x, float y, float z); // Move along our "local" axes (respect rotation)
	void Rotate(float pitch, float yaw, float roll);
	void Scale(float x, float y, float z);

	void AddChild(std::shared_ptr<Transform> child);
	void RemoveChild(std::shared_ptr<Transform> child);
	void SetParent(std::shared_ptr<Transform> newParent);
	std::shared_ptr<Transform> GetParent();
	std::shared_ptr<Transform> GetChild(unsigned int index);
	unsigned int GetChildCount();

	std::vector<std::shared_ptr<GameEntity>> GetChildrenAsGameEntities();

	void SetEnableDisable(bool value);
};

