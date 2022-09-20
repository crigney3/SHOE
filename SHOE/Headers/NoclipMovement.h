#pragma once

#include "IComponent.h"
#include <DirectXMath.h>

class NoclipMovement : public IComponent
{
public:
	float moveSpeed;
	float lookSpeed;
private:
	void Start() override;
	void Update() override;
};

