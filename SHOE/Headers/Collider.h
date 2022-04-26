#pragma once

#include "Transform.h"
#include "GameEntity.fwd.h"
#include <memory>
#include "DirectXCollision.h"

class Collider : public IComponent, public std::enable_shared_from_this<Collider>
{
public:
	void Start() override;
	void Update() override;
	void OnDestroy() override;

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
	bool GetTransformVisibilityStatus();
	void SetTriggerStatus(bool _isTrigger);
	void SetVisibilityStatus(bool _isVisible);
	void SetTransformVisibilityStatus(bool _isTransformVisible);

	void OnCollisionEnter(std::shared_ptr<GameEntity> other) override;
	void OnTriggerEnter(std::shared_ptr<GameEntity> other) override;

private:
	DirectX::BoundingOrientedBox obb_;

	bool isTrigger_;
	bool isVisible_;
	bool isTransformVisible_;
};
