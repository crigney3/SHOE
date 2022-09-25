#include "../Headers/AudioHandler.h"
#include "../Headers/AssetManager.h"

using namespace FMOD;

AudioHandler* AudioHandler::instance;

AudioHandler::~AudioHandler() {
	soundSystem->release();
	activeChannels->release();
}

FMOD_RESULT AudioHandler::Initialize() {
	FMOD_RESULT result;
	
	result = FMOD::System_Create(&soundSystem);
	if (result != FMOD_OK) {
		return result;
	}

	result = soundSystem->init(512, FMOD_INIT_NORMAL, 0);

	if (result != FMOD_OK) {
		return result;
	}

	result = soundSystem->createChannelGroup("Main Channel", &activeChannels);

	return result;
}

Sound* AudioHandler::LoadSound(std::string soundPath, FMOD_MODE mode) {
	FMOD::Sound* newSound;
	FMOD_RESULT result;

	result = soundSystem->createSound(soundPath.c_str(),
									  mode,
									  0,
									  &newSound);

	if (result != FMOD_OK) return nullptr;

	return newSound;
}

Channel* AudioHandler::LoadSoundAndInitChannel(std::string soundPath, FMOD_MODE mode) {
	FMOD_RESULT result;
	Channel* channel;
	FMOD::Sound* newSound;
	FMOD_SYNCPOINT* startSync;
	FMOD_SYNCPOINT* endSync;
	unsigned int soundLength;
	std::string startSyncName;
	std::string endSyncName;

	result = soundSystem->createSound(soundPath.c_str(),
		mode,
		0,
		&newSound);

	if (result != FMOD_OK) return nullptr;

	result = this->soundSystem->playSound(newSound,
		0,
		true,
		&channel);

	if (result == FMOD_OK) {
		channel->setChannelGroup(activeChannels);
	}

	allChannels.push_back(channel);

	if (result != FMOD_OK) return nullptr;

	newSound->getLength(&soundLength, FMOD_TIMEUNIT_MS);

	startSyncName = soundPath + "start";
	endSyncName = soundPath + "end";

	newSound->addSyncPoint(0, FMOD_TIMEUNIT_MS, startSyncName.c_str(), &startSync);
	//newSound->addSyncPoint(soundLength, FMOD_TIMEUNIT_MS, endSyncName.c_str(), &endSync);

	channel->setCallback(ComponentSignalCallback);

	return channel;
}

/// <summary>
/// Deprecated - avoid use
/// </summary>
//Channel* AudioHandler::BasicPlaySound(FMOD::Sound* sound, bool isPaused) {
//	FMOD_RESULT result;
//	Channel* channel;
//	
//	result = this->soundSystem->playSound(sound,
//										  0,
//										  isPaused,
//										  &channel);
//
//	if (result == FMOD_OK) {
//		channel->setChannelGroup(activeChannels);
//	}
//
//	FMODUserData* uData;
//	FMOD_RESULT uDataResult = sound->getUserData((void**)&uData);
//	AssetManager::GetInstance().BroadcastGlobalEntityEvent(EntityEventType::OnAudioPlay, std::make_shared<AudioEventPacket>(*uData->name, channel, nullptr));
//
//	return channel;
//}

Channel* AudioHandler::BasicPlaySound(FMOD::Channel* channel, bool isPaused) {
	FMOD_RESULT result;
	EntityEventType eType;
	FMOD::Sound* currentSound;

	result = channel->getCurrentSound(&currentSound);

	if (result != FMOD_OK) return nullptr;

	result = channel->setPaused(isPaused);

	if (result != FMOD_OK) return nullptr;

	eType = isPaused ? EntityEventType::OnAudioPause : EntityEventType::OnAudioPlay;

	FMODUserData* uData;
	FMOD_RESULT uDataResult = currentSound->getUserData((void**)&uData);
	AssetManager::GetInstance().BroadcastGlobalEntityEvent(eType, std::make_shared<AudioEventPacket>(*uData->name, channel, nullptr));

	return channel;
}

FMOD::System* AudioHandler::GetSoundSystem() {
	return this->soundSystem;
}

FMOD::ChannelGroup* AudioHandler::GetActiveChannels() {
	return this->activeChannels;
}

FMOD::Channel* AudioHandler::GetChannelByIndex(int index) {
	return this->allChannels[index];
}

FMOD::Channel* AudioHandler::GetChannelBySound(FMOD::Sound* sound) {
	// Match the incoming pointer.
	FMOD::Sound* matchSound;
	for (int i = 0; i < allChannels.size(); i++) {
		allChannels[i]->getCurrentSound(&matchSound);
		if (matchSound == sound) {
			return allChannels[i];
		}
	}
}

size_t AudioHandler::GetChannelVectorLength() {
	return this->allChannels.size();
}

FMOD_RESULT F_CALLBACK ComponentSignalCallback(FMOD_CHANNELCONTROL* channelControl,
	FMOD_CHANNELCONTROL_TYPE controlType,
	FMOD_CHANNELCONTROL_CALLBACK_TYPE callbackType,
	void* commanddata1,
	void* commanddata2)
{
	FMOD::Sound* currentSound;
	FMOD::Channel* channel;
	EntityEventType eType;
	bool isPaused;

	if (controlType == FMOD_CHANNELCONTROL_CHANNEL) {
		channel = (FMOD::Channel*)channelControl;
		channel->getCurrentSound(&currentSound);
	}
	else {
		// This is a channel group. unimplemented for now
		return FMOD_OK;
	}

	if (callbackType == FMOD_CHANNELCONTROL_CALLBACK_END) {
		eType = EntityEventType::OnAudioPause;
	}
	else {
		channel->getPaused(&isPaused);

		eType = isPaused ? EntityEventType::OnAudioPause : EntityEventType::OnAudioPlay;
	}

	FMODUserData* uData;
	FMOD_RESULT uDataResult = currentSound->getUserData((void**)&uData);
	AssetManager::GetInstance().BroadcastGlobalEntityEvent(eType, std::make_shared<AudioEventPacket>(*uData->name, channel, nullptr));
}

float* AudioHandler::getTrackBPM(FMOD::Sound* sound){
    float *speed;
    FMOD_RESULT result;
    result = sound->getMusicSpeed(speed);
    if (result != FMOD_OK) return nullptr;
    return speed;
}