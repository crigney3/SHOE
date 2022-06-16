#pragma once

#include "Transform.h"
#include "AudioHandler.h"

class AudioComponent : public IComponent
{
public:
	FMOD::Sound* GetSound();
	void SetSound(FMOD::Sound* newSound);
	void SetSound(std::string newSoundPath);

	float GetVolume();
	void SetVolume(float volume);

	float GetPlaybackSpeed();
	void SetPlaybackSpeed(float speed);

protected:
	void Start() override;
private:
	// Each audio component stores a reference to exactly one sound
	FMOD::Sound* sound;

	// The component uses this for most operations
	AudioHandler& audioHandler = AudioHandler::GetInstance();
};