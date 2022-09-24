#pragma once
#include "IComponent.h"

enum class AudioEventTrigger {
	FrequencyAbove,
	FrequencyBelow,
	PitchAbove,
	PitchBelow,
	AudioEventTriggerCount
};

enum class AudioEventResponse {
	Move,
	Rotate,
	Scale,
	ChangeLightIntensity,
	ChangeLightColor,
	AudioEventResponseCount
};

class AudioResponse : public IComponent
{
public:
	std::string audioName;
	AudioEventTrigger trigger;
	float triggerComparison;
	AudioEventResponse response;
	DirectX::XMFLOAT3 data;

	void SetLinkedSound(FMOD::Sound* sound);
	void SetLinkedSound(FMOD::Channel* channel);

private:
	AudioHandler& audioInstance;

	bool canTrigger;

	void Start() override;
	void Update() override;
	void OnAudioPlay(AudioEventPacket audio) override;
	void OnAudioPause(AudioEventPacket audio) override;

	bool IsTriggered();
	void TriggerResponse();

	FMOD::Sound* linkedSound;
	FMOD::Channel* linkedChannel;
};

