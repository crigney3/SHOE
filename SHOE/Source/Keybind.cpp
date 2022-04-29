#include "../Headers/Keybind.h"

Keybind::Keybind()
{
	Unbind();
}

/// <summary>
/// Cleans up remaining references
/// </summary>
Keybind::~Keybind()
{
	if (this->triggerByPress)
		delete[] this->triggerByPress;
	if (this->triggerByRelease)
		delete[] this->triggerByRelease;
	if (this->triggerByDown)
		delete[] this->triggerByDown;
	if (this->mustHaveUp)
		delete[] this->mustHaveUp;
	if (this->mustHaveDown)
		delete[] this->mustHaveDown;
}

/// <summary>
/// Defines the keyboard state needed to trigger this keybind
/// </summary>
/// <param name="triggerByPressCt">Size of the following array</param>
/// <param name="triggerByPress">Array of characters whose press can trigger this keybind</param>
/// <param name="triggerByReleaseCt">Size of the following array</param>
/// <param name="triggerByRelease">Array of characters whose release can trigger this keybind</param>
/// <param name="triggerByDownCt">Size of the following array</param>
/// <param name="triggerByDown">Array of characters who, when held, can trigger this keybind</param>
/// <param name="mustHaveUpCt">Size of the following array</param>
/// <param name="mustHaveUp">Array of characters who must be up for this keybind to trigger</param>
/// <param name="mustHaveDownCt">Size of the following array</param>
/// <param name="mustHaveDown">Array of characters who must be up for this keybind to trigger</param>
void Keybind::Bind(int triggerByPressCt, int* triggerByPress, int triggerByReleaseCt, int* triggerByRelease, int triggerByDownCt, int* triggerByDown, int mustHaveUpCt, int* mustHaveUp, int mustHaveDownCt, int* mustHaveDown)
{
	if(this->triggerByPress)
		delete[] this->triggerByPress;
	this->triggerByPress = triggerByPress;
	this->triggerByPressCt = triggerByPressCt;

	if (this->triggerByRelease)
		delete[] this->triggerByRelease;
	this->triggerByRelease = triggerByRelease;
	this->triggerByReleaseCt = triggerByReleaseCt;

	if (this->triggerByDown)
		delete[] this->triggerByDown;
	this->triggerByDown = triggerByDown;
	this->triggerByDownCt = triggerByDownCt;

	if (this->mustHaveUp)
		delete[] this->mustHaveUp;
	this->mustHaveUp = mustHaveUp;
	this->mustHaveUpCt = mustHaveUpCt;

	if (this->mustHaveDown)
		delete[] this->mustHaveDown;
	this->mustHaveDown = mustHaveDown;
	this->mustHaveDownCt = mustHaveDownCt;
}

/// <summary>
/// Wipes the keybind to keep it from triggering
/// </summary>
void Keybind::Unbind()
{
	triggerByPressCt = 0;
	triggerByReleaseCt = 0;
	triggerByDownCt = 0;
	mustHaveUpCt = 0;
	mustHaveDownCt = 0;
}
