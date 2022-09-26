#pragma once

#include <fmod.hpp>
#include <fmod_common.h>
#include <string>
#include <vector>
#include "DXCore.h"

#define FMOD_RESULT_CHECK(x) do { FMOD_RESULT status = (x); if (status != FMOD_RESULT::FMOD_OK) return status; } while(0)

FMOD_RESULT F_CALLBACK ComponentSignalCallback(FMOD_CHANNELCONTROL* channelControl,
	FMOD_CHANNELCONTROL_TYPE controlType,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType,
	void* commandData1,
	void* commandData2);

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
	std::vector<FMOD::DSP*> allDSPs;

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

	int GetChannelIndexBySoundName(std::string soundName);
	int GetChannelIndexBySound(FMOD::Sound* sound);

	float* GetTrackBPM(FMOD::Sound* sound);

	bool HasSoundEnded(FMOD::Channel* channel);

	size_t GetChannelVectorLength();
	size_t GetDSPVectorLength();

	FMOD::DSP* GetSpectrumReaderDSP();
	float* GetFrequencyArrayFromChannel(FMOD::Channel* channel, int sampleLength);
	std::vector<float> GetFrequencyVectorFromChannel(FMOD::Channel* channel, int sampleLength);

	float GetFrequencyAtCurrentFrame(FMOD::Channel* channel);
};