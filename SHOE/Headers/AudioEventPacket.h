#pragma once
#include <string>
#include <memory>
#include "GameEntity.fwd.h"

class AudioEventPacket
{
private:
	std::string fileName;
	std::shared_ptr<GameEntity> broadcastingEntity;

public:
	AudioEventPacket(std::string fileName, std::shared_ptr<GameEntity> broadcastingEntity = nullptr);

	std::string GetFileName();
	std::shared_ptr<GameEntity> GetBroadcastingEntity();
	bool IsGlobalAudio();
};

