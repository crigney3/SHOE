#pragma once

#include <fmod.hpp>
#include <fmod_common.h>
#include <string>
#include <vector>
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

	FMOD::ChannelGroup* activeChannels;
	std::vector<FMOD::Channel*> allChannels;

	friend class AudioResponse;

public:
	~AudioHandler();

	FMOD_RESULT Initialize();

	FMOD::Sound* LoadSound(std::string soundPath, FMOD_MODE mode);
	FMOD::Channel* LoadSoundAndInitChannel(std::string soundPath, FMOD_MODE mode);
	FMOD::Channel* BasicPlaySound(FMOD::Sound* sound, bool isPaused = false);
	FMOD::Channel* BasicPlaySound(FMOD::Channel* channel, bool isPaused = false);

	FMOD::System* GetSoundSystem();

	FMOD::ChannelGroup* GetActiveChannels();
	FMOD::Channel* GetChannelByIndex(int index);
	FMOD::Channel* GetChannelBySound(FMOD::Sound* sound);
	FMOD::Channel* GetChannelBySoundName(std::string soundName);

	size_t GetChannelVectorLength();
};