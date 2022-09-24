#include "../Headers/AudioResponse.h"
#include "../Headers/GameEntity.h"
#include "../Headers/AudioHandler.h"

void AudioResponse::Start()
{
	audioName = "";
	trigger = AudioEventTrigger::FrequencyAbove;
	response = AudioEventResponse::Move;
	data = DirectX::XMFLOAT3(0, 0, 0);
}

void AudioResponse::Update()
{
	if (canTrigger && IsTriggered()) {
		TriggerResponse();
	}
}

/// <summary>
/// Allows the trigger controller to be polled
/// </summary>
void AudioResponse::OnAudioPlay(AudioEventPacket audio)
{
	if(audio.GetFileName() == audioName &&
	   audio.GetAudioChannel() == this->linkedChannel)
		canTrigger = true;
}

/// <summary>
/// Stops the trigger condition from being polled
/// </summary>
void AudioResponse::OnAudioPause(AudioEventPacket audio)
{
	if (audio.GetFileName() == audioName)
		canTrigger = false;
}

/// <summary>
/// Checks to see if the trigger condition is met
/// </summary>
bool AudioResponse::IsTriggered()
{
	bool triggerResponse;
	float freq;
	float pitch;
	switch (trigger) {
	case AudioEventTrigger::FrequencyAbove:		
		linkedChannel->getFrequency(&freq);
		triggerResponse = freq >= triggerComparison;
		break;
	case AudioEventTrigger::FrequencyBelow:
		linkedChannel->getFrequency(&freq);
		triggerResponse = freq <= triggerComparison;
		break;
	case AudioEventTrigger::PitchAbove:
		linkedChannel->getPitch(&pitch);
		triggerResponse = pitch >= triggerComparison;
		break;
	case AudioEventTrigger::PitchBelow:
		linkedChannel->getPitch(&pitch);
		triggerResponse = pitch <= triggerComparison;
		break;
	}
	return triggerResponse;
}

/// <summary>
/// Triggers the proper response to the trigger condition being met
/// </summary>
void AudioResponse::TriggerResponse()
{
	switch (response) {
	case AudioEventResponse::Move:
		GetTransform()->MoveRelative(data);
		break;
	case AudioEventResponse::Rotate:
		GetTransform()->Rotate(data);
		break;
	case AudioEventResponse::Scale:
		GetTransform()->Scale(data);
		break;
	case AudioEventResponse::ChangeLightIntensity:
		GetGameEntity()->GetComponent<Light>()->SetIntensity(data.x);
		break;
	case AudioEventResponse::ChangeLightColor:
		GetGameEntity()->GetComponent<Light>()->SetColor(data);
		break;
	}
}

/// <summary>
/// Set the linked sound for this component/object using a sound.
/// Currently broken/unimplemented.
/// </summary>
//void AudioResponse::SetLinkedSound(FMOD::Sound* sound) {
//	this->linkedSound = sound;
//}

/// <summary>
/// Set the linked sound for this component/object using a channel.
/// </summary>
void AudioResponse::SetLinkedSound(FMOD::Channel* channel) {
	FMOD::Sound* sound;
	FMODUserData* uData;

	channel->getCurrentSound(&sound);
	this->linkedSound = sound;
	this->linkedChannel = channel;

	sound->getUserData((void**)&uData);
	this->audioName = *(uData->name.get());
}
