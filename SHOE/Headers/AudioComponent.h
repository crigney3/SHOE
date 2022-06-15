#pragma once

#include "Transform.h"
#include "AudioHandler.h"

class AudioComponent : public IComponent
{
public:

protected:
	void Start() override;
private:
	// Each audio component stores a reference to exactly one sound
	FMOD::Sound** sound;

	// The component uses this for most operations
	AudioHandler& audioHandler = AudioHandler::GetInstance();
}