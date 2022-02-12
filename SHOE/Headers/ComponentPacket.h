#pragma once
#include "IComponent.h"
#include <functional>

struct ComponentPacket {
	IComponent* component;
	std::function<void(IComponent*)> deallocator;

	ComponentPacket(IComponent* component, std::function<void(IComponent*)> deallocator)
	{
		this->component = component;
		this->deallocator = deallocator;
	}
};