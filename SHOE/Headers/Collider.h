#pragma once

#include "Transform.h"
#include "GameEntity.fwd.h"
#include <memory>
#include "DirectXCollision.h"

class Collider : public IComponent, public std::enable_shared_from_this<Collider>
{
public:
	void OnDestroy() override;

	DirectX::BoundingOrientedBox GetOrientedBoundingBox();

	// Extents Get/Set
	DirectX::XMFLOAT3 GetPositionOffset();
	void SetPositionOffset(DirectX::XMFLOAT3 posOffset);
	DirectX::XMFLOAT3 GetRotationOffset();
	void SetRotationOffset(DirectX::XMFLOAT3 rotOffset);
	DirectX::XMFLOAT3 GetScale();
	void SetScale(DirectX::XMFLOAT3 scale);
	DirectX::XMFLOAT4X4 GetWorldMatrix();

	// Bool Get/Sets
	bool IsTrigger();
	bool IsVisible();
	void SetIsTrigger(bool _isTrigger);
	void SetVisible(bool _isVisible);

private:
	void RegenerateBoundingBox();

	void Start() override;
	void OnCollisionEnter(std::shared_ptr<GameEntity> other) override;
	void OnTriggerEnter(std::shared_ptr<GameEntity> other) override;
	void InCollision(std::shared_ptr<GameEntity> other) override;
	void InTrigger(std::shared_ptr<GameEntity> other) override;
	void OnCollisionExit(std::shared_ptr<GameEntity> other) override;
	void OnTriggerExit(std::shared_ptr<GameEntity> other) override;
	void OnTransform() override;
	void OnParentTransform(std::shared_ptr<GameEntity> parent) override;

	std::shared_ptr<Transform> offset;
	DirectX::BoundingOrientedBox obb_;

	bool isTrigger_;
	bool isVisible_;
};
