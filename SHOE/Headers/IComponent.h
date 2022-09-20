#pragma once
#include <memory>
#include <DirectXMath.h>
#include "GameEntity.fwd.h"
#include "AudioEventPacket.h"

class Transform;

enum EntityEventType {
	Update,
	EditingUpdate,
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
	InCollision,
	InTrigger,
	OnCollisionExit,
	OnTriggerExit,
	OnAudioLoad,
	OnAudioPlay,
	OnAudioPause,
	OnAudioEnd,
	EventCount,
	REQUIRES_MESSAGE = 0b11111111111111111100000
};

class IComponent
{
public:
	void ReceiveEvent(EntityEventType event, std::shared_ptr<void> message = nullptr);

	void Bind(std::shared_ptr<GameEntity> gameEntity);
	void Free();
	virtual void OnDestroy();

	bool IsEnabled();
	bool IsLocallyEnabled();
	void SetEnabled(bool enabled);
	std::shared_ptr<GameEntity> GetGameEntity();
	std::shared_ptr<Transform> GetTransform();
protected:
	virtual void Start();
	virtual void Update();
	virtual void EditingUpdate();
	virtual void OnCollisionEnter(std::shared_ptr<GameEntity> other);
	virtual void OnTriggerEnter(std::shared_ptr<GameEntity> other);
	virtual void InCollision(std::shared_ptr<GameEntity> other);
	virtual void InTrigger(std::shared_ptr<GameEntity> other);
	virtual void OnCollisionExit(std::shared_ptr<GameEntity> other);
	virtual void OnTriggerExit(std::shared_ptr<GameEntity> other);
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
	virtual void OnAudioLoad(AudioEventPacket audio);
	virtual void OnAudioPlay(AudioEventPacket audio);
	virtual void OnAudioPause(AudioEventPacket audio);
	virtual void OnAudioEnd(AudioEventPacket audio);
private:
	std::shared_ptr<GameEntity> gameEntity;

	bool enabled = true;
};