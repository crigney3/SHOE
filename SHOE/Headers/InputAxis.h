#pragma once
#include "Keybind.h"

/// <summary>
/// Defines all possible input axes
/// </summary>
enum class InputAxes {
	MovementAdvance,
	MovementStrafe,
	MovementY,
	Length //A trick to get the length of an enum
};

struct InputAxis
{
	KeyActions positiveAxis;
	KeyActions negativeAxis;
	bool bound;

	InputAxis();
	void Bind(KeyActions positiveAction, KeyActions negativeAction);
	void Unbind();
};
