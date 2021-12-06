#pragma once

#include <fmod.hpp>
#include <fmod_common.h>
#include <string>
#include "DXCore.h"

class AudioHandler
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static AudioHandler& GetInstance()
	{
		if (!instance)
		{
			instance = new AudioHandler();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	AudioHandler(AudioHandler const&) = delete;
	void operator=(AudioHandler const&) = delete;

private:
	static AudioHandler* instance;
	AudioHandler()
	{

	};
#pragma endregion

private:

	FMOD::System* soundSystem;
public:
	~AudioHandler();

	FMOD_RESULT Initialize();

	FMOD::Sound* LoadSound(std::string soundPath, FMOD_MODE mode);
	FMOD::Channel* BasicPlaySound(FMOD::Sound* sound);

	FMOD::System* GetSoundSystem();
};