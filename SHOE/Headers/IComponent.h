#pragma once
#include <memory>
#include <DirectXMath.h>
#include "GameEntity.fwd.h"

class Transform;

class IComponent
{
public:
	virtual void Start();
	virtual void Update(float deltaTime, float totalTime);
	virtual void OnDestroy();
	virtual void OnCollisionEnter(std::shared_ptr<GameEntity> other);
	virtual void OnTriggerEnter(std::shared_ptr<GameEntity> other);
	virtual void OnTransform();
	virtual void OnParentTransform();
	virtual void OnMove(DirectX::XMFLOAT3 delta);
	virtual void OnRotate(DirectX::XMFLOAT3 delta);
	virtual void OnScale(DirectX::XMFLOAT3 delta);
	virtual void OnEnabledChanged(bool newState);

	void Bind(std::shared_ptr<GameEntity> gameEntity, bool hierarchyIsEnabled);
	void Free();

	bool IsEnabled();
	bool IsLocallyEnabled();
	void SetEnabled(bool enabled);
	std::shared_ptr<GameEntity> GetGameEntity();
	std::shared_ptr<Transform> GetTransform();
	void UpdateHierarchyIsEnabled(bool active);
private:
	std::shared_ptr<GameEntity> gameEntity;

	bool enabled = true;
	bool hierarchyIsEnabled = true;
};