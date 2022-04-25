#pragma once
#include <memory>
#include <DirectXMath.h>
#include "GameEntity.fwd.h"

class Transform;

enum EntityEventType {
	Update,
	OnEnable,
	OnDisable,
	OnTransform,
	OnMove,
	OnRotate,
	OnScale,
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
	REQUIRES_MESSAGE = 0b1111111111110000
};

class IComponent
{
public:
	void ReceiveEvent(EntityEventType event, std::shared_ptr<void> message = nullptr);

	virtual void Start();
	virtual void Update();
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
	virtual void OnEnable();
	virtual void OnDisable();

	void Bind(std::shared_ptr<GameEntity> gameEntity);
	void Free();

	bool IsEnabled();
	bool IsLocallyEnabled();
	void SetEnabled(bool enabled);
	std::shared_ptr<GameEntity> GetGameEntity();
	std::shared_ptr<Transform> GetTransform();
private:
	std::shared_ptr<GameEntity> gameEntity;

	bool enabled = true;
};