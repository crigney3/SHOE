#include "../Headers/AudioHandler.h"

using namespace FMOD;

AudioHandler* AudioHandler::instance;

AudioHandler::~AudioHandler() {
	soundSystem->release();
}

FMOD_RESULT AudioHandler::Initialize() {
	FMOD_RESULT result;
	
	result = FMOD::System_Create(&soundSystem);
	if (result != FMOD_OK) {
		return result;
	}

	result = soundSystem->init(512, FMOD_INIT_NORMAL, 0);

	globalSounds = std::vector<FMOD::Sound*>();

	return result;
}

Sound* AudioHandler::LoadSound(std::string soundPath, FMOD_MODE mode) {
	FMOD::Sound* newSound;
	FMOD_RESULT result;

	result = soundSystem->createSound(soundPath.c_str(),
									  mode,
									  0,
									  &newSound);

	globalSounds.push_back(newSound);

	if (result != FMOD_OK) return nullptr;

	return newSound;
}

Channel* AudioHandler::BasicPlaySound(FMOD::Sound* sound) {
	FMOD_RESULT result;
	Channel* channel;
	
	result = this->soundSystem->playSound(sound,
										  0,
										  false,
										  &channel);

	return channel;
}

FMOD::System* AudioHandler::GetSoundSystem() {
	return this->soundSystem;
}