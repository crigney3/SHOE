#include "..\Headers\InputAxis.h"

InputAxis::InputAxis()
{
	bound = false;
}

void InputAxis::Bind(KeyActions positiveAction, KeyActions negativeAction)
{
	positiveAxis = positiveAction;
	negativeAxis = negativeAction;
	bound = true;
}

void InputAxis::Unbind()
{
	bound = false;
}
