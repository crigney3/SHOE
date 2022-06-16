#pragma once

#include <fmod.hpp>
#include <fmod_common.h>
#include <string>
#include <vector>
#include <memory>
#include "DXCore.h"

struct FMODUserData {
	// More data soon
	std::shared_ptr<std::string> name;
	int fileKeyIndex;
};

struct StaticSoundData {
	// Possibly more data soon
	std::shared_ptr<std::string> filenameKey;
};

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

	std::vector<FMOD::Sound*> globalSounds;
	// Stores effects such as reverb
	std::vector<FMOD::DSP*> globalDSPs;

	FMOD::Sound* LoadSound(std::string soundPath, FMOD_MODE mode = FMOD_DEFAULT, std::string name = "");
	FMOD::Channel* BasicPlaySound(FMOD::Sound* sound);

	std::string GetSoundName(FMOD::Sound* sound);
	FMODUserData* GetSoundUserData(FMOD::Sound* sound);

	size_t GetSoundArraySize();

	FMOD::System* GetSoundSystem();
};