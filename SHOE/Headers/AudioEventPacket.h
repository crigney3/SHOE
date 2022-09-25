#pragma once
#include <string>
#include <memory>
#include "GameEntity.fwd.h"
#include "AudioHandler.fwd.h"
#include <fmod.hpp>
#include <fmod_common.h>

class AudioEventPacket
{
private:
	std::string fileName;
	std::shared_ptr<GameEntity> broadcastingEntity;
	FMOD::Channel* audioChannel;

public:
	AudioEventPacket(std::string fileName, FMOD::Channel* channel, std::shared_ptr<GameEntity> broadcastingEntity = nullptr);

	std::string GetFileName();
	std::shared_ptr<GameEntity> GetBroadcastingEntity();
	bool IsGlobalAudio();
	FMOD::Channel* GetAudioChannel();
};

