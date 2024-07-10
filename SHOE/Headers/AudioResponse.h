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

enum class AudioResponseContinuityMode {
	Once,
	LoopXTimes,
	UntilIntervalStop,
	UntilAudioStop,
	Inifinite,
	AudioResponseContinuityModeCount
};

enum class AudioResponseMathModifier {
	Additive,
	Multiplicative,
	Subtractive,
	Divisive,
	AudioResponseMathModifierCount
};

class AudioResponse : public IComponent
{
public:
	std::string audioName;
	AudioEventTrigger trigger;
	float triggerComparison;
	AudioEventResponse response;
	DirectX::XMFLOAT3 data;
	AudioResponseContinuityMode continuityType;
	AudioResponseMathModifier operatorType;

	//void SetLinkedSound(FMOD::Sound* sound);
	void SetLinkedSound(FMOD::Channel* channel);

private:
	bool canTrigger;

	void Start() override;
	void Update() override;
	void EditingUpdate() override;
	void OnAudioPlay(AudioEventPacket audio) override;
	void OnAudioPause(AudioEventPacket audio) override;

	bool IsTriggered();
	void TriggerResponse();

	FMOD::Sound* linkedSound;
	FMOD::Channel* linkedChannel;
};

