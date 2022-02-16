#pragma once
#include <functional>

class IComponent;

struct ComponentPacket {
	std::shared_ptr<IComponent> component;
	std::function<void(std::shared_ptr<IComponent>)> deallocator;

	ComponentPacket(std::shared_ptr<IComponent> component, std::function<void(std::shared_ptr<IComponent>)> deallocator)
	{
		this->component = component;
		this->deallocator = deallocator;
	}
};