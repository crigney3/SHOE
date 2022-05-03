#include "../Headers/NoclipMovement.h"
#include "../Headers/Time.h"
#include "../Headers/Input.h"
#include "../Headers/Transform.h"

void NoclipMovement::Start()
{
	this->lookSpeed = 3.0f;
	this->moveSpeed = 10.0f;
}

void NoclipMovement::Update()
{
	float speed = Time::deltaTime * moveSpeed;

	Input& input = Input::GetInstance();

	// Speed up or down as necessary
	if (input.KeyDown(VK_SHIFT)) { speed *= 5; }
	if (input.KeyDown(VK_CONTROL)) { speed *= 0.1f; }

	GetTransform()->MoveRelative(input.TestInputAxis(InputAxes::MovementStrafe) * speed, 0, input.TestInputAxis(InputAxes::MovementAdvance) * speed);
	GetTransform()->MoveAbsolute(0, input.TestInputAxis(InputAxes::MovementY) * speed, 0);

	if (input.MouseLeftDown())
	{
		float xDiff = this->lookSpeed * Time::deltaTime * input.GetMouseXDelta();
		float yDiff = this->lookSpeed * Time::deltaTime * input.GetMouseYDelta();

		//TODO: Fix gimbal lock
		GetTransform()->Rotate(yDiff, xDiff, 0);
		/*if (GetTransform()->GetLocalPitchYawRoll().y >= XM_PIDIV2) {
			GetTransform()->SetRotation(GetTransform()->GetLocalPitchYawRoll().x, XM_PIDIV2 - 0.007f, GetTransform()->GetLocalPitchYawRoll().z);
		}
		else if (GetTransform()->GetLocalPitchYawRoll().y <= -XM_PIDIV2) {
			GetTransform()->SetRotation(GetTransform()->GetLocalPitchYawRoll().x, -XM_PIDIV2 + 0.007f, GetTransform()->GetLocalPitchYawRoll().z);
		}*/
	}
}
