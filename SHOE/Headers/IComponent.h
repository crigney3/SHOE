#pragma once
#include <memory>
#include <DirectXMath.h>
#include "GameEntity.fwd.h"

class Transform;

enum EntityEventType {
	Update,
	OnTransform,
	OnMove,
	OnRotate,
	OnScale,
	OnEnabledChanged,
	OnParentTransform,
	OnParentMove,
	OnParentRotate,
	OnParentScale,
	OnParentEnabledChanged,
	OnCollisionEnter,
	OnTriggerEnter,
	AudioPlay,
	AudioStop,
	EventCount,
	REQUIRES_MESSAGE = 0b111111111111100,
	IGNORES_ENABLED_STATE = 0b000010000100000
};

class IComponent
{
public:
	void RecieveEvent(EntityEventType event, std::shared_ptr<void> message = nullptr);

	virtual void Start();
	virtual void Update(float deltaTime, float totalTime);
	virtual void OnDestroy();
	virtual void OnCollisionEnter(std::shared_ptr<GameEntity> other);
	virtual void OnTriggerEnter(std::shared_ptr<GameEntity> other);
	virtual void OnTransform();
	virtual void OnMove(DirectX::XMFLOAT3 delta);
	virtual void OnRotate(DirectX::XMFLOAT3 delta);
	virtual void OnScale(DirectX::XMFLOAT3 delta);
	virtual void OnParentTransform(std::shared_ptr<GameEntity> parent);
	virtual void OnParentMove(std::shared_ptr<GameEntity> parent);
	virtual void OnParentRotate(std::shared_ptr<GameEntity> parent);
	virtual void OnParentScale(std::shared_ptr<GameEntity> parent);
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