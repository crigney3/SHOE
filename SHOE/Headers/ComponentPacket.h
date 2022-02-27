#pragma once
#include <functional>

class IComponent;

/**
 * \brief Stores a component reference and the method to call to free it
 */
struct ComponentPacket {
	std::shared_ptr<IComponent> component;
	//The "Free" call for the given component type's pool
	std::function<void(std::shared_ptr<IComponent>)> deallocator;

	ComponentPacket(std::shared_ptr<IComponent> component, std::function<void(std::shared_ptr<IComponent>)> deallocator)
	{
		this->component = component;
		this->deallocator = deallocator;
	}
};