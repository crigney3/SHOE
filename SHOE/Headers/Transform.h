#pragma once

#include <DirectXMath.h>
#include <vector>
#include "GameEntity.fwd.h"
#include "IComponent.h"

class Transform : public IComponent,  public std::enable_shared_from_this<Transform>
{
private:
	std::shared_ptr<Transform> parent;
	std::vector<std::shared_ptr<Transform>> children = std::vector<std::shared_ptr<Transform>>();
	std::vector<std::shared_ptr<GameEntity>> childEntites = std::vector<std::shared_ptr<GameEntity>>();

	bool globalsDirty;
	DirectX::XMFLOAT3 worldPos;
	DirectX::XMFLOAT3 worldScale;
	DirectX::XMFLOAT4 worldRotQuat;

	// Raw transformation data
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 pitchYawRoll;
	DirectX::XMFLOAT3 scale;

	// Local orientation vectors
	bool vectorsDirty;
	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 forward;

	// World matrix and inverse transpose of the world matrix
	bool matricesDirty;
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldInverseTransposeMatrix;

	// Helper to update both matrices if necessary
	void UpdateMatrices();
	void UpdateVectors();
	void UpdateGlobals();

	void SetTransformsFromMatrix(DirectX::XMFLOAT4X4 worldMatrix);

	// Helpers for conversion
	DirectX::XMFLOAT3 QuaternionToEuler(DirectX::XMFLOAT4 quaternion);
public:
	void Start() override;
	void OnDestroy() override;

	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void MoveRelative(float x, float y, float z); // Move along our "local" axes (respect rotation)
	void MoveRelative(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 pitchYawRoll);
	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 scale);

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 pos);
	void SetRotation(float pitch, float yaw, float roll);
	void SetRotation(DirectX::XMFLOAT3 rot);
	void SetScale(float x, float y, float z);
	void SetScale(DirectX::XMFLOAT3 scale);

	DirectX::XMFLOAT3 GetLocalPosition();
	DirectX::XMFLOAT3 GetGlobalPosition();
	DirectX::XMFLOAT3 GetLocalPitchYawRoll();
	DirectX::XMFLOAT4 GetGlobalRotation();
	DirectX::XMFLOAT3 GetLocalScale();
	DirectX::XMFLOAT3 GetGlobalScale();

	// Local direction vector getters
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetForward();

	// Matrix getters
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

	void MarkMatricesDirty();
	void MarkVectorsDirty();
	void MarkGlobalsDirty();

	void AddChild(std::shared_ptr<Transform> child);
	void RemoveChild(std::shared_ptr<Transform> child);
	void SetParent(std::shared_ptr<Transform> newParent);
	std::shared_ptr<Transform> GetParent();
	std::shared_ptr<Transform> GetChild(unsigned int index);
	unsigned int GetChildCount();

	std::vector<std::shared_ptr<GameEntity>> GetChildrenEntities();
};

