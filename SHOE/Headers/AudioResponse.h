#pragma once
#include "IComponent.h"

enum class AudioEventTrigger {
	FrequencyAbove,
	FrequencyBelow,
	PitchAbove,
	PitchBelow
};

enum class AudioEventResponse {
	Move,
	Rotate,
	Scale,
	ChangeLightIntensity,
	ChangeLightColor,
};

class AudioResponse : public IComponent
{
public:
	std::string audioName;
	AudioEventTrigger trigger;
	AudioEventResponse response;
	DirectX::XMFLOAT3 data;

private:
	bool canTrigger;

	void Start() override;
	void Update() override;
	void OnAudioPlay(AudioEventPacket audio) override;
	void OnAudioPause(AudioEventPacket audio) override;

	bool IsTriggered();
	void TriggerResponse();
};

