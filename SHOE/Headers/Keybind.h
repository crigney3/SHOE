#pragma once

/// <summary>
/// Defines all possible keybindable actions
/// </summary>
enum KeyActions {
    QuitGame,
    MoveForward,
    MoveBack,
    StrafeLeft,
    StrafeRight,
    MoveUp,
    MoveDown,
    SprintModifier,
    SneakModifier,
    SwitchSkyboxPrev,
    SwitchSkyboxNext,
    ToggleFlashlight,
    ToggleFlashlightFlicker,
    ObjectSelectionClickModifier,
    Length, //A trick to get the length of an enum
};

/// <summary>
/// Data for a single complex keybind
/// </summary>
class Keybind
{
public:
    //The keybinding will trigger if any of these keys were pressed this frame and the "must" conditions are met
    int triggerByPressCt;
    int* triggerByPress = nullptr;
    //The keybinding will trigger if any of these keys were released this frame and the "must" conditions are met
    int triggerByReleaseCt;
    int* triggerByRelease = nullptr;
    //The keybinding will trigger if any of these keys are down this frame and the "must" conditions are met
    int triggerByDownCt;
    int* triggerByDown = nullptr;
    //All of these keys must be up for any of the triggers to work
    int mustHaveUpCt;
    int* mustHaveUp = nullptr;
    //All of these keys must be down for any of the triggers to work
    int mustHaveDownCt;
    int* mustHaveDown = nullptr;

    Keybind();

    ~Keybind();

    void Bind(
        int triggerByPressCt = 0, int* triggerByPress = {},
        int triggerByReleaseCt = 0, int* triggerByRelease = {},
        int triggerByDownCt = 0, int* triggerByDown = {},
        int mustHaveUpCt = 0, int* mustHaveUp = {},
        int mustHaveDownCt = 0, int* mustHaveDown = {}
    );

    void Unbind();
};

