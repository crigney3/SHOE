#include "../Headers/AudioHandler.h"
#include "../Headers/AssetManager.h"

using namespace FMOD;

AudioHandler* AudioHandler::instance;

AudioHandler::~AudioHandler() {
	FMOD::ChannelGroup* mainGroup;
	soundSystem->getMasterChannelGroup(&mainGroup);

	for (int i = 0; i < allDSPs.size(); i++) {
		mainGroup->removeDSP(allDSPs[i]);
		allDSPs[i]->release();
	}

	activeChannels->release();
	soundSystem->close();
	soundSystem->release();
}

FMOD_RESULT AudioHandler::Initialize() {
	allChannels = std::vector<FMOD::Channel*>();
	allDSPs = std::vector<FMOD::DSP*>();

	FMOD_RESULT_CHECK(FMOD::System_Create(&soundSystem));

	FMOD_RESULT_CHECK(soundSystem->init(512, FMOD_INIT_NORMAL, 0));

	FMOD_RESULT_CHECK(soundSystem->createChannelGroup("Main Channel", &activeChannels));

	FMOD_RESULT_CHECK(InitializeDSPs());

	return FMOD_OK;
}

FMOD_RESULT AudioHandler::InitializeDSPs() {
	FMOD::DSP* wavedataDSP;
	FMOD::ChannelGroup* mainGroup;
	FMOD_DSP_DESCRIPTION dspdesc;
	FMOD_DSP_PARAMETER_DESC wavedata_desc;
	FMOD_DSP_PARAMETER_DESC volume_desc;
	FMOD_DSP_PARAMETER_DESC* paramdesc[2] =
	{
		&wavedata_desc,
		&volume_desc
	};

	RtlZeroMemory(&dspdesc, sizeof(dspdesc));
	FMOD_DSP_INIT_PARAMDESC_DATA(wavedata_desc, "wave data", "", "wave data", FMOD_DSP_PARAMETER_DATA_TYPE_USER);
	FMOD_DSP_INIT_PARAMDESC_FLOAT(volume_desc, "volume", "%", "linear volume in percent", 0, 1, 1);

	strncpy_s(dspdesc.name, "Wavedata DSP", sizeof(dspdesc.name));
	dspdesc.version = 0x00010000;
	dspdesc.numinputbuffers = 1;
	dspdesc.numoutputbuffers = 1;
	dspdesc.read = DefaultDSPCallback;
	dspdesc.create = CreateDSPCallback;
	dspdesc.release = ReleaseDSPCallback;
	dspdesc.getparameterdata = GetParameterDataDSPCallback;
	dspdesc.setparameterfloat = SetParameterFloatDSPCallback;
	dspdesc.getparameterfloat = GetParameterFloatDSPCallback;
	dspdesc.numparameters = 2;
	dspdesc.paramdesc = paramdesc;

	FMOD_RESULT_CHECK(soundSystem->createDSP(&dspdesc, &wavedataDSP));
	allDSPs.push_back(wavedataDSP);

	FMOD_RESULT_CHECK(soundSystem->getMasterChannelGroup(&mainGroup));

	FMOD_RESULT_CHECK(mainGroup->addDSP(0, wavedataDSP));

	return FMOD_OK;
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

	//channel->addDSP(1, allDSPs[0]);

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

	if (HasSoundEnded(channel)) {
		int deadChannelIndex = GetChannelIndexBySound(currentSound);
		soundSystem->playSound(currentSound, 0, true, &channel);
		allChannels[deadChannelIndex] = channel;
		channel->setPosition(0, FMOD_TIMEUNIT_MS);
	}

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

int AudioHandler::GetChannelIndexBySoundName(std::string soundName) {
	FMOD::Sound* matchSound;
	FMODUserData* uData;
	for (int i = 0; i < allChannels.size(); i++) {
		allChannels[i]->getCurrentSound(&matchSound);
		matchSound->getUserData((void**)&uData);
		if (uData->name.get()->c_str() == soundName) {
			return i;
		}
	}
}

int AudioHandler::GetChannelIndexBySound(FMOD::Sound* sound) {
	// Match the incoming pointer.
	FMOD::Sound* matchSound;
	for (int i = 0; i < allChannels.size(); i++) {
		allChannels[i]->getCurrentSound(&matchSound);
		if (matchSound == sound) {
			return i;
		}
	}
}

size_t AudioHandler::GetChannelVectorLength() {
	return this->allChannels.size();
}

bool AudioHandler::HasSoundEnded(FMOD::Channel* channel) {
	unsigned int length;
	unsigned int currentPosition;
	FMOD::Sound* sound;

	channel->getCurrentSound(&sound);
	sound->getLength(&length, FMOD_TIMEUNIT_MS);
	channel->getPosition(&currentPosition, FMOD_TIMEUNIT_MS);

	return currentPosition >= length;
}

float* AudioHandler::GetTrackBPM(FMOD::Sound* sound) {
    float speed = 0;
    FMOD_RESULT result;
    result = sound->getMusicSpeed(&speed);
    if (result != FMOD_OK) return nullptr;
    return &speed;
}

FMOD_RESULT AudioHandler::GetFrequencyArrayFromChannel(FMOD::Channel* channel, float** freqArray, int sampleLength) {
	FMOD::DSP* spectrumDSP;
	std::vector<float> frequencies;
	FMOD_RESULT result;
	char volstr[32] = { 0 };
	FMOD_DSP_PARAMETER_DESC* desc;
	FMODDSPData* data;

	spectrumDSP = allDSPs[0];

	FMOD_RESULT_CHECK(spectrumDSP->getParameterInfo(1, &desc));

	FMOD_RESULT_CHECK(spectrumDSP->getParameterFloat(1, 0, volstr, 32));

	FMOD_RESULT_CHECK(spectrumDSP->getParameterData(0, (void**)&data, 0, 0, 0));

	if (data->channels)
	{
		int channel;

		for (channel = 0; channel < data->channels; channel++)
		{
			int count, level;

			for (count = 0; count < data->length_samples; count++)
			{
				frequencies.push_back(data->buffer[(count * data->channels) + channel]);
			}
		}
	}

	*freqArray = frequencies.data();

	return FMOD_OK;
}

FMOD_RESULT AudioHandler::GetFrequencyVectorFromChannel(FMOD::Channel* channel, std::vector<float>* freqVector, int sampleLength) {
	FMOD::DSP* spectrumDSP;
	std::vector<float> frequencies;
	FMOD_RESULT result;
	char volstr[32] = { 0 };
	FMOD_DSP_PARAMETER_DESC* desc;
	FMODDSPData* data;

	spectrumDSP = allDSPs[0];

	FMOD_RESULT_CHECK(spectrumDSP->getParameterInfo(1, &desc));

	FMOD_RESULT_CHECK(spectrumDSP->getParameterFloat(1, 0, volstr, 32));

	FMOD_RESULT_CHECK(spectrumDSP->getParameterData(0, (void**)&data, 0, 0, 0));

	if (data->channels)
	{
		int channel;

		for (channel = 0; channel < data->channels; channel++)
		{
			int count, level;

			for (count = 0; count < data->length_samples; count++)
			{
				freqVector->push_back(fabs(data->buffer[(count * data->channels) + channel]));
			}
		}
	}

	return FMOD_OK;
}

FMOD_RESULT AudioHandler::GetFrequencyAtCurrentFrame(Channel* channel, float* currentFrequency) {
	FMODUserData* uData;
	Sound* sound = {};
	unsigned int currentPosition;
	
	FMOD_RESULT_CHECK(channel->getCurrentSound(&sound));

	FMOD_RESULT_CHECK(sound->getUserData((void**)&uData));

	FMOD_RESULT_CHECK(channel->getPosition(&currentPosition, FMOD_TIMEUNIT_MS));

	*currentFrequency = uData->waveform[currentPosition / uData->waveformSampleLength];

	return FMOD_OK;
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

/*

	All the below callback functions are based on teh design of an FMOD demo written in a forum post by the CEO of FMOD.
	https://qa.fmod.com/t/visualizing-waveform-in-fmod-5/12954/16

*/
FMOD_RESULT F_CALLBACK DefaultDSPCallback(FMOD_DSP_STATE* dsp_state,
														float* inbuffer,
														float* outbuffer,
														unsigned int length,
														int inchannels,
														int* outchannels)
{
	FMODDSPData* data = (FMODDSPData*)dsp_state->plugindata;

	/*
		This loop assumes inchannels = outchannels, which it will be if the DSP is created with '0'
		as the number of channels in FMOD_DSP_DESCRIPTION.
		Specifying an actual channel count will mean you have to take care of any number of channels coming in,
		but outputting the number of channels specified. Generally it is best to keep the channel
		count at 0 for maximum compatibility.
	*/
	for (unsigned int samp = 0; samp < length; samp++)
	{
		/*
			Feel free to unroll this.
		*/
		for (int chan = 0; chan < *outchannels; chan++)
		{
			/*
				This DSP filter just halves the volume!
				Input is modified, and sent to output.
			*/
			data->buffer[(samp * *outchannels) + chan] = outbuffer[(samp * inchannels) + chan] = inbuffer[(samp * inchannels) + chan] * data->volume_linear;
		}
	}

	data->channels = inchannels;

	return FMOD_OK;
}

/*
	Callback called when DSP is created.   This implementation creates a structure which is attached to the dsp state's 'plugindata' member.
*/
FMOD_RESULT F_CALLBACK CreateDSPCallback(FMOD_DSP_STATE* dsp_state)
{
	unsigned int blocksize;
	FMOD_RESULT result;

	FMOD_RESULT_CHECK(dsp_state->functions->getblocksize(dsp_state, &blocksize));

	FMODDSPData* data = (FMODDSPData*)calloc(sizeof(FMODDSPData), 1);
	if (!data)
	{
		return FMOD_ERR_MEMORY;
	}
	dsp_state->plugindata = data;
	data->volume_linear = 1.0f;
	data->length_samples = blocksize;

	data->buffer = (float*)malloc(blocksize * 8 * sizeof(float));      // *8 = maximum size allowing room for 7.1.   Could ask dsp_state->functions->getspeakermode for the right speakermode to get real speaker count.
	if (!data->buffer)
	{
		return FMOD_ERR_MEMORY;
	}

	return FMOD_OK;
}

/*
	Callback called when DSP is destroyed.   The memory allocated in the create callback can be freed here.
*/
FMOD_RESULT F_CALLBACK ReleaseDSPCallback(FMOD_DSP_STATE* dsp_state)
{
	if (dsp_state->plugindata)
	{
		FMODDSPData* data = (FMODDSPData*)dsp_state->plugindata;

		if (data->buffer)
		{
			free(data->buffer);
		}

		free(data);
	}

	return FMOD_OK;
}

/*
	Callback called when DSP::getParameterData is called.   This returns a pointer to the raw floating point PCM data.
	We have set up 'parameter 0' to be the data parameter, so it checks to make sure the passed in index is 0, and nothing else.
*/
FMOD_RESULT F_CALLBACK GetParameterDataDSPCallback(FMOD_DSP_STATE* dsp_state, int index, void** data, unsigned int* length, char*)
{
	if (index == 0)
	{
		unsigned int blocksize;
		FMOD_RESULT result;
		FMODDSPData* mydata = (FMODDSPData*)dsp_state->plugindata;

		FMOD_RESULT_CHECK(dsp_state->functions->getblocksize(dsp_state, &blocksize));

		*data = (void*)mydata;
		*length = blocksize * 2 * sizeof(float);

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

/*
	Callback called when DSP::setParameterFloat is called.   This accepts a floating point 0 to 1 volume value, and stores it.
	We have set up 'parameter 1' to be the volume parameter, so it checks to make sure the passed in index is 1, and nothing else.
*/
FMOD_RESULT F_CALLBACK SetParameterFloatDSPCallback(FMOD_DSP_STATE* dsp_state, int index, float value)
{
	if (index == 1)
	{
		FMODDSPData* mydata = (FMODDSPData*)dsp_state->plugindata;

		mydata->volume_linear = value;

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}

/*
	Callback called when DSP::getParameterFloat is called.   This returns a floating point 0 to 1 volume value.
	We have set up 'parameter 1' to be the volume parameter, so it checks to make sure the passed in index is 1, and nothing else.
	An alternate way of displaying the data is provided, as a string, so the main app can use it.
*/
FMOD_RESULT F_CALLBACK GetParameterFloatDSPCallback(FMOD_DSP_STATE* dsp_state, int index, float* value, char* valstr)
{
	if (index == 1)
	{
		FMODDSPData* mydata = (FMODDSPData*)dsp_state->plugindata;

		*value = mydata->volume_linear;
		if (valstr)
		{
			sprintf_s(valstr, sizeof(valstr), "%d", (int)((*value * 100.0f) + 0.5f));
		}

		return FMOD_OK;
	}

	return FMOD_ERR_INVALID_PARAM;
}
