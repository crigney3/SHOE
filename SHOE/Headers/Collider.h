#pragma once

#include "Transform.h"
#include "GameEntity.fwd.h"
#include <memory>
#include "DirectXCollision.h"

class Collider
{
public:
	Collider();
	void Start() override;

	std::shared_ptr<GameEntity> GetOwner();
	void SetOwner(std::shared_ptr<GameEntity> _newOwner);

	std::shared_ptr<Transform> GetTransform();
	void SetPosition(DirectX::XMFLOAT3 _newPos);

	DirectX::BoundingOrientedBox GetOrientedBoundingBox();

	// Extents Get/Set
	DirectX::XMFLOAT3 GetExtents();
	void SetExtents(DirectX::XMFLOAT3 _newExtents);
	void SetXExtent(float _x);
	void SetYExtent(float _y);
	void SetZExtent(float _z);
	void SetWidth(float _width);
	void SetHeight(float _height);
	void SetDepth(float _depth);
	// Bool Get/Sets
	bool GetTriggerStatus();
	bool GetVisibilityStatus();
	bool GetEnabledStatus();
	void SetTriggerStatus(bool _isTrigger);
	void SetVisibilityStatus(bool _isVisible);
	void SetEnabledStatus(bool _isEnabled);

	void OnTriggerEnter(GameEntity& _other);
	void OnCollisionEnter(GameEntity& _other);

private:
	std::shared_ptr<GameEntity> owner_;
	DirectX::BoundingOrientedBox obb_;
	std::shared_ptr<Transform> transform_;

	bool isEnabled_;
	bool isTrigger_;
	bool isVisible_;
};

