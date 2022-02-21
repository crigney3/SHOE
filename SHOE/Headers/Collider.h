#pragma once

#include "Transform.h"
#include "GameEntity.fwd.h"
#include <memory>

class Collider
{
public:
	Collider(GameEntity& _owner);

	std::shared_ptr<GameEntity> GetOwner();
	void SetOwner(std::shared_ptr<GameEntity> _newOwner);

	std::shared_ptr<Transform> GetTransform();
	void SetTransform(std::shared_ptr<Transform>);

	/**
	 * \brief Extents describe a "radius" of how far the box extends on each axis
	 * \return XMFLOAT3 of half of the width, length, and height of the box-volume
	 */
	DirectX::XMFLOAT3 GetExtents();

	void SetExtents(DirectX::XMFLOAT3 _newExtents);

	void SetXExtent(float _x);
	void SetYExtent(float _y);
	void SetZExtent(float _z);
	void SetWidth(float _width);
	void SetHeight(float _height);
	void SetDepth(float _depth);

	void SetTriggerStatus(bool _isTrigger);
	void SetVisibilityStatus(bool _isVisible);
	void SetEnabledStatus(bool _isEnabled);

	void OnTriggerEnter(GameEntity& _other);
	void OnCollisionEnter(GameEntity& _other);

private:
	std::shared_ptr<GameEntity> owner_;
	std::shared_ptr<Transform> transform_;
	DirectX::XMFLOAT3 extentsXYZ_;

	bool isEnabled_;
	bool isTrigger_;
	bool isVisible_;
};

