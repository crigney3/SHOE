#include "../Headers/AudioResponse.h"
#include "../Headers/GameEntity.h"

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
	if(audio.GetFileName() == audioName)
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
	switch (trigger) {
	case AudioEventTrigger::FrequencyAbove:
		//return AudioHandler::GetFrequency(audioName) >= testValue;
		break;
	case AudioEventTrigger::FrequencyBelow:
		break;
	case AudioEventTrigger::PitchAbove:
		break;
	case AudioEventTrigger::PitchBelow:
		break;
	}
	return false;
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
