#include "../Headers/AudioComponent.h"

using namespace FMOD;

void AudioComponent::Start() {

}

FMOD::Sound* AudioComponent::GetSound() {
	return sound;
}

void AudioComponent::SetSound(FMOD::Sound* newSound) {
	this->sound = newSound;
}

void AudioComponent::SetSound(std::string newSoundPath) {
	this->sound = audioHandler.LoadSound(newSoundPath);
}

float AudioComponent::GetVolume() {
	float vol;
	this->sound->getMusicChannelVolume(0, &vol);
	return vol;
}

void AudioComponent::SetVolume(float volume) {
	this->sound->setMusicChannelVolume(0, volume);
}

float AudioComponent::GetPlaybackSpeed() {
	float speed;
	this->sound->getMusicSpeed(&speed);
	return speed;
}

void AudioComponent::SetPlaybackSpeed(float speed) {
	this->sound->setMusicSpeed(speed);
}