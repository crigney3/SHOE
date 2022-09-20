#include "../Headers/AudioEventPacket.h"

AudioEventPacket::AudioEventPacket(std::string fileName, std::shared_ptr<GameEntity> broadcastingEntity) 
{
	this->fileName = fileName;
	this->broadcastingEntity = broadcastingEntity;
}

/// <summary>
/// Get the name of the audio that's being played
/// </summary>
std::string AudioEventPacket::GetFileName()
{
	return fileName;
}

/// <summary>
/// Get what entity the sound is being played from
/// </summary>
std::shared_ptr<GameEntity> AudioEventPacket::GetBroadcastingEntity()
{
	return broadcastingEntity;
}

/// <summary>
/// A null broadcasting entity means the sound is global
/// </summary>
bool AudioEventPacket::IsGlobalAudio()
{
	return broadcastingEntity == nullptr;
}
